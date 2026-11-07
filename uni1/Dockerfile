FROM python:3.11-slim

WORKDIR /app

# CPU-only PyTorch keeps the image portable. GPU operators can build a CUDA
# variant using the matching PyTorch wheel for their host driver/runtime.
RUN pip install --no-cache-dir --index-url https://download.pytorch.org/whl/cpu torch==2.5.1     && pip install --no-cache-dir        "transformers>=4.45,<5" "peft>=0.13" "accelerate>=1.0"        "fastapi>=0.115" "uvicorn[standard]>=0.30" "pydantic>=2.8"

COPY server.py /app/server.py

ENV UNI1_ADAPTER_DIR=/models/uni1-adapter
ENV UNI1_QUOTA_DB=/data/uni1-quota.sqlite3
ENV UNI1_MAX_NEW_TOKENS=4096

EXPOSE 8000
CMD ["uvicorn", "server:app", "--host", "0.0.0.0", "--port", "8000"]
