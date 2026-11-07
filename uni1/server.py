#!/usr/bin/env python3
"""Self-hosted Uni-1 inference API; no third-party model API key is used."""

from __future__ import annotations

import json
import math
import os
import sqlite3
import threading
from contextlib import contextmanager
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Literal

from fastapi import FastAPI, Header, HTTPException
from pydantic import BaseModel, Field

try:
    from .draft_validation import validate_generated_level
    from .object_catalog import relevant_object_catalog
    from .prompts import SYSTEM_PROMPT
except ImportError:  # uvicorn server:app when launched from the uni1 directory
    from draft_validation import validate_generated_level
    from object_catalog import relevant_object_catalog
    from prompts import SYSTEM_PROMPT

BASE_MODEL = os.getenv("UNI1_BASE_MODEL", "Qwen/Qwen2.5-0.5B-Instruct")
ADAPTER_DIR = Path(os.getenv("UNI1_ADAPTER_DIR", "artifacts/uni1-adapter"))
DATABASE = Path(os.getenv("UNI1_QUOTA_DB", "uni1-quota.sqlite3"))
MAX_PROMPT_CHARS = 12000
MAX_INPUT_TOKENS = 8192
MAX_NEW_TOKENS = int(os.getenv("UNI1_MAX_NEW_TOKENS", "4096"))
DAILY_DEFAULT = 100
DAILY_BONUS = 1000
app = FastAPI(title="Uni-1 Inference", version="0.1.0")
model_lock = threading.Lock()
model = None
tokenizer = None
torch = None
recent_requests: dict[str, list[float]] = {}
rate_lock = threading.Lock()


class Message(BaseModel):
    role: Literal["user", "assistant", "system"]
    content: str = Field(min_length=1, max_length=MAX_PROMPT_CHARS)


class ChatRequest(BaseModel):
    model: Literal["uni-1"] = "uni-1"
    messages: list[Message] = Field(min_length=1, max_length=12)
    max_tokens: int = Field(default=2048, ge=64, le=MAX_NEW_TOKENS)
    temperature: float = Field(default=0.4, ge=0.0, le=1.5)


@contextmanager
def db():
    DATABASE.parent.mkdir(parents=True, exist_ok=True)
    connection = sqlite3.connect(DATABASE, timeout=15)
    try:
        connection.execute("""
            CREATE TABLE IF NOT EXISTS usage (
                install_id TEXT PRIMARY KEY,
                day TEXT NOT NULL,
                requests INTEGER NOT NULL DEFAULT 0,
                taps INTEGER NOT NULL DEFAULT 0,
                bonus INTEGER NOT NULL DEFAULT 0
            )
        """)
        yield connection
        connection.commit()
    except Exception:
        connection.rollback()
        raise
    finally:
        connection.close()


def today():
    return datetime.now(timezone.utc).date().isoformat()


def get_usage(install_id: str):
    with db() as connection:
        row = connection.execute(
            "SELECT day, requests, taps, bonus FROM usage WHERE install_id = ?",
            (install_id,),
        ).fetchone()
        current_day = today()
        if row is None:
            connection.execute(
                "INSERT INTO usage VALUES (?, ?, 0, 0, 0)",
                (install_id, current_day),
            )
            return {"day": current_day, "requests": 0, "taps": 0, "bonus": 0}
        day, requests, taps, bonus = row
        if day != current_day:
            connection.execute(
                "UPDATE usage SET day=?, requests=0 WHERE install_id=?",
                (current_day, install_id),
            )
            requests = 0
        return {"day": current_day, "requests": requests, "taps": taps, "bonus": bonus}


def consume_request(install_id: str):
    with db() as connection:
        connection.execute("BEGIN IMMEDIATE")
        row = connection.execute(
            "SELECT day, requests, bonus FROM usage WHERE install_id=?",
            (install_id,),
        ).fetchone()
        current_day = today()
        if row is None:
            connection.execute("INSERT INTO usage VALUES (?, ?, 0, 0, 0)", (install_id, current_day))
            requests, bonus = 0, 0
        else:
            day, requests, bonus = row
            if day != current_day:
                requests = 0
        limit = DAILY_BONUS if bonus else DAILY_DEFAULT
        if requests >= limit:
            connection.rollback()
            raise HTTPException(status_code=429, detail=f"Uni-1 daily limit reached ({limit}).")
        connection.execute(
            "INSERT INTO usage(install_id, day, requests, taps, bonus) VALUES (?, ?, ?, 0, ?) "
            "ON CONFLICT(install_id) DO UPDATE SET "
            "day=excluded.day, requests=excluded.requests, bonus=excluded.bonus",
            (install_id, current_day, requests + 1, bonus),
        )
        connection.commit()
        return requests + 1, limit


def require_install_id(value: str | None):
    if value is None or len(value) < 32 or len(value) > 64:
        raise HTTPException(status_code=400, detail="Missing or invalid X-Uni1-Install-ID.")
    if any(char not in "0123456789abcdefABCDEF-" for char in value):
        raise HTTPException(status_code=400, detail="Installation ID must be a random hexadecimal identifier.")
    return value


@app.on_event("startup")
def load_model():
    global model, tokenizer, torch
    # Keep quota and API-contract tests lightweight: model runtimes are only
    # imported when the service actually starts loading trained weights.
    try:
        import torch as torch_runtime
        from peft import PeftModel
        from transformers import AutoModelForCausalLM, AutoTokenizer
    except ImportError as error:
        raise RuntimeError("Install the Uni-1 inference requirements before loading model weights.") from error
    torch = torch_runtime
    if not ADAPTER_DIR.is_dir():
        raise RuntimeError(f"Uni-1 adapter not found at {ADAPTER_DIR}. Train it before starting the server.")
    manifest_path = ADAPTER_DIR / "uni1-manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError("Uni-1 adapter is missing uni1-manifest.json; unverified weights cannot be served.")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        from .model_manifest import validate_model_manifest
    except ImportError:
        from model_manifest import validate_model_manifest
    try:
        validate_model_manifest(manifest, expected_base_model=BASE_MODEL)
    except ValueError as error:
        raise RuntimeError(f"Uni-1 adapter manifest rejected: {error}") from error
    tokenizer = AutoTokenizer.from_pretrained(ADAPTER_DIR, use_fast=True)
    if tokenizer.pad_token_id is None:
        tokenizer.pad_token = tokenizer.eos_token
    base = AutoModelForCausalLM.from_pretrained(
        BASE_MODEL,
        torch_dtype="auto",
        device_map="auto",
    )
    model = PeftModel.from_pretrained(base, ADAPTER_DIR)
    model.eval()


@app.get("/health")
def health():
    if model is None or tokenizer is None:
        raise HTTPException(status_code=503, detail="Uni-1 model is not loaded.")
    return {"status": "ready", "model": "uni-1"}


@app.get("/v1/usage")
def usage(x_uni1_install_id: str | None = Header(default=None)):
    install_id = require_install_id(x_uni1_install_id)
    state = get_usage(install_id)
    limit = DAILY_BONUS if state["bonus"] else DAILY_DEFAULT
    return {"used": state["requests"], "limit": limit, "taps": state["taps"], "bonus_unlocked": bool(state["bonus"])}


@app.post("/v1/bonus/tap")
def bonus_tap(x_uni1_install_id: str | None = Header(default=None)):
    install_id = require_install_id(x_uni1_install_id)
    with db() as connection:
        connection.execute("BEGIN IMMEDIATE")
        row = connection.execute(
            "SELECT day, taps, bonus FROM usage WHERE install_id=?",
            (install_id,),
        ).fetchone()
        if row is None:
            connection.execute("INSERT INTO usage VALUES (?, ?, 0, 0, 0)", (install_id, today()))
            taps, bonus = 0, 0
        else:
            old_day, taps, bonus = row
            if old_day != today():
                connection.execute(
                    "UPDATE usage SET day=?, requests=0 WHERE install_id=?",
                    (today(), install_id),
                )
        if not bonus:
            taps = min(7, taps + 1)
            bonus = int(taps >= 7)
            connection.execute(
                "UPDATE usage SET day=?, taps=?, bonus=? WHERE install_id=?",
                (today(), taps, bonus, install_id),
            )
        connection.commit()
    return {"taps": taps, "bonus_unlocked": bool(bonus), "daily_limit": DAILY_BONUS if bonus else DAILY_DEFAULT}


def parse_model_draft(answer: str):
    """Parse and production-validate one model response."""
    try:
        candidate = json.loads(answer)
    except json.JSONDecodeError as error:
        raise ValueError(f"invalid JSON: {error.msg}") from error
    try:
        return validate_generated_level(candidate)
    except ValueError as error:
        raise ValueError(f"invalid level draft: {error}") from error


@app.post("/v1/chat/completions")
def chat_completions(payload: ChatRequest, x_uni1_install_id: str | None = Header(default=None)):
    install_id = require_install_id(x_uni1_install_id)
    if model is None or tokenizer is None:
        raise HTTPException(status_code=503, detail="Uni-1 model is not loaded.")
    prompt_chars = sum(len(message.content) for message in payload.messages if message.role != "system")
    if prompt_chars == 0 or prompt_chars > MAX_PROMPT_CHARS:
        raise HTTPException(status_code=413, detail=f"Combined user and assistant prompt content must be between 1 and {MAX_PROMPT_CHARS} characters.")
    now = time.monotonic()
    with rate_lock:
        recent = [stamp for stamp in recent_requests.get(install_id, []) if now - stamp < 60]
        if len(recent) >= 10:
            raise HTTPException(status_code=429, detail="Uni-1 short-term rate limit reached.")
        recent.append(now)
        recent_requests[install_id] = recent
        if len(recent_requests) > 10000:
            for key in list(recent_requests)[:2000]:
                recent_requests.pop(key, None)
    messages = [{"role": "system", "content": SYSTEM_PROMPT}]
    messages.extend(message.model_dump() for message in payload.messages if message.role != "system")
    user_context = "\n".join(message.content for message in payload.messages if message.role == "user")
    object_catalog = relevant_object_catalog(user_context)
    if object_catalog:
        for message in reversed(messages):
            if message["role"] == "user":
                message["content"] += (
                    "\n\nVERIFIED EDITOR OBJECT CATALOGUE (name = numeric object ID):\n"
                    + object_catalog
                    + "\nUse these exact IDs; this catalogue does not specify trigger property layouts."
                )
                break
    request_messages = messages
    for attempt in range(2):
        prompt = tokenizer.apply_chat_template(
            request_messages, tokenize=False, add_generation_prompt=True
        )
        inputs = tokenizer(prompt, return_tensors="pt")
        prompt_tokens = int(inputs["input_ids"].shape[1])
        if prompt_tokens > MAX_INPUT_TOKENS:
            raise HTTPException(
                status_code=413,
                detail=f"Prompt exceeds the {MAX_INPUT_TOKENS}-token Uni-1 input limit.",
            )
        inputs = inputs.to(model.device)
        if attempt == 0:
            used, limit = consume_request(install_id)
        with model_lock, torch.inference_mode():
            output = model.generate(
                **inputs,
                max_new_tokens=payload.max_tokens,
                temperature=max(payload.temperature, 1e-5),
                do_sample=payload.temperature > 0,
                pad_token_id=tokenizer.eos_token_id,
                eos_token_id=tokenizer.eos_token_id,
            )
        generated = output[0][inputs["input_ids"].shape[1]:]
        raw_answer = tokenizer.decode(generated, skip_special_tokens=True).strip()
        try:
            draft, generated_count = parse_model_draft(raw_answer)
            break
        except ValueError as error:
            if attempt == 1:
                raise HTTPException(
                    status_code=502,
                    detail=f"Uni-1 could not produce a valid level draft after one repair attempt: {error}",
                ) from error
            request_messages = messages + [
                {"role": "assistant", "content": raw_answer[:16000]},
                {
                    "role": "user",
                    "content": (
                        "Your previous response was rejected by the production level validator: "
                        + str(error)[:1000]
                        + ". Correct the draft while preserving its design. Return ONLY one complete "
                        "valid JSON object matching the Universe level-draft schema. Keep 6 to 8 sections "
                        "and 96 to 144 ordered placements. Do not invent object IDs or unsupported properties."
                    ),
                },
            ]
    answer = json.dumps(draft, ensure_ascii=False, separators=(",", ":"))
    return {
        "id": f"uni1-{int(time.time())}",
        "object": "chat.completion",
        "model": "uni-1",
        "choices": [{"index": 0, "message": {"role": "assistant", "content": answer}, "finish_reason": "stop"}],
        "usage": {"prompt_tokens": int(inputs["input_ids"].shape[1]), "completion_tokens": int(generated.shape[0]), "total_tokens": int(output.shape[1])},
        "uni1_quota": {"used": used, "limit": limit},
        "uni1_generated_objects": generated_count,
        "uni1_repair_attempts": attempt,
    }
