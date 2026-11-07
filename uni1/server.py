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

import torch
from fastapi import FastAPI, Header, HTTPException, Request
from pydantic import BaseModel, Field
from peft import PeftModel
from transformers import AutoModelForCausalLM, AutoTokenizer

try:
    from .draft_validation import validate_draft_payload
except ImportError:  # uvicorn server:app when launched from the uni1 directory
    from draft_validation import validate_draft_payload

BASE_MODEL = os.getenv("UNI1_BASE_MODEL", "Qwen/Qwen2.5-0.5B-Instruct")
ADAPTER_DIR = Path(os.getenv("UNI1_ADAPTER_DIR", "artifacts/uni1-adapter"))
DATABASE = Path(os.getenv("UNI1_QUOTA_DB", "uni1-quota.sqlite3"))
MAX_PROMPT_CHARS = 12000
MAX_NEW_TOKENS = int(os.getenv("UNI1_MAX_NEW_TOKENS", "4096"))
DAILY_DEFAULT = 100
DAILY_BONUS = 1000
SYSTEM_PROMPT = (
    "You are Uni-1, Universe's Geometry Dash level-design model. "
    "Return only valid JSON for the Universe level-draft schema. "
    "Use only block, spike, saw, orb, pad, and deco placements. "
    "Create coherent, reasonably playable layouts and never claim they were playtested."
)

app = FastAPI(title="Uni-1 Inference", version="0.1.0")
model_lock = threading.Lock()
model = None
tokenizer = None
recent_requests: dict[str, list[float]] = {}
rate_lock = threading.Lock()


class Message(BaseModel):
    role: Literal["user", "assistant", "system"]
    content: str = Field(min_length=1, max_length=MAX_PROMPT_CHARS)


class ChatRequest(BaseModel):
    model: str = "uni-1"
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
    global model, tokenizer
    if not ADAPTER_DIR.is_dir():
        raise RuntimeError(f"Uni-1 adapter not found at {ADAPTER_DIR}. Train it before starting the server.")
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


@app.post("/v1/chat/completions")
def chat_completions(payload: ChatRequest, request: Request, x_uni1_install_id: str | None = Header(default=None)):
    install_id = require_install_id(x_uni1_install_id)
    if model is None or tokenizer is None:
        raise HTTPException(status_code=503, detail="Uni-1 model is not loaded.")
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
    used, limit = consume_request(install_id)

    messages = [{"role": "system", "content": SYSTEM_PROMPT}]
    messages.extend(message.model_dump() for message in payload.messages if message.role != "system")
    prompt = tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
    inputs = tokenizer(prompt, return_tensors="pt").to(model.device)
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
    answer = tokenizer.decode(generated, skip_special_tokens=True).strip()
    try:
        draft = json.loads(answer)
    except json.JSONDecodeError as error:
        raise HTTPException(status_code=502, detail=f"Uni-1 produced invalid JSON: {error.msg}") from error
    try:
        draft, generated_count = validate_draft_payload(draft)
    except ValueError as error:
        raise HTTPException(status_code=502, detail=f"Uni-1 returned an invalid level draft: {error}") from error
    answer = json.dumps(draft, ensure_ascii=False, separators=(",", ":"))
    return {
        "id": f"uni1-{int(time.time())}",
        "object": "chat.completion",
        "model": "uni-1",
        "choices": [{"index": 0, "message": {"role": "assistant", "content": answer}, "finish_reason": "stop"}],
        "usage": {"prompt_tokens": int(inputs["input_ids"].shape[1]), "completion_tokens": int(generated.shape[0]), "total_tokens": int(output.shape[1])},
        "uni1_quota": {"used": used, "limit": limit},
        "uni1_generated_objects": generated_count,
    }
