"""
Hunyuan3D generator client.

Handles submit → poll → download for Tencent Hunyuan 3D generation.
Reads credentials from env vars (TENCENT_SECRET_ID, TENCENT_SECRET_KEY)
or falls back to .mcp4d_config.json in the plugin root.
"""

import json
import os
import time
from pathlib import Path

from tencentcloud.common import credential
from tencentcloud.common.profile.client_profile import ClientProfile
from tencentcloud.common.profile.http_profile import HttpProfile
from tencentcloud.ai3d.v20250513 import ai3d_client, models


CONFIG_PATH = Path(__file__).parent.parent / ".mcp4d_config.json"
TEMP_DIR = Path(__file__).parent.parent / "temp"


def _get_credentials() -> tuple[str, str]:
    """Get SecretId and SecretKey from env vars or config file."""
    secret_id = os.environ.get("TENCENT_SECRET_ID", "")
    secret_key = os.environ.get("TENCENT_SECRET_KEY", "")

    if secret_id and secret_key:
        return secret_id, secret_key

    if CONFIG_PATH.exists():
        config = json.loads(CONFIG_PATH.read_text())
        secret_id = config.get("secret_id", "")
        secret_key = config.get("secret_key", "")
        if secret_id and secret_key:
            return secret_id, secret_key

    raise ValueError("No Tencent Cloud credentials found. Set TENCENT_SECRET_ID/TENCENT_SECRET_KEY env vars or configure via MCP4D dialog.")


def save_config(secret_id: str, secret_key: str, region: str = "ap-singapore",
                endpoint: str = "hunyuan.intl.tencentcloudapi.com"):
    """Save credentials to config file."""
    config = {
        "secret_id": secret_id,
        "secret_key": secret_key,
        "region": region,
        "endpoint": endpoint,
    }
    CONFIG_PATH.write_text(json.dumps(config, indent=2))


def _get_client() -> ai3d_client.Ai3dClient:
    """Create a Tencent Cloud Hunyuan client."""
    secret_id, secret_key = _get_credentials()

    # Read region from config, endpoint is always the ai3d service
    region = "ap-singapore"
    endpoint = "ai3d.tencentcloudapi.com"

    if CONFIG_PATH.exists():
        config = json.loads(CONFIG_PATH.read_text())
        region = config.get("region", region)

    cred = credential.Credential(secret_id, secret_key)
    httpProfile = HttpProfile()
    httpProfile.endpoint = endpoint
    clientProfile = ClientProfile()
    clientProfile.httpProfile = httpProfile

    return ai3d_client.Ai3dClient(cred, region, clientProfile)


def generate(prompt: str = "", image_base64: str = "", image_url: str = "",
             enable_pbr: bool = True, face_count: int = 50000,
             model: str = "3.0", poll_interval: float = 5.0,
             max_wait: float = 300.0) -> dict:
    """
    Submit a Hunyuan3D generation job and wait for the result.

    Args:
        prompt: Text description for 3D generation
        image_base64: Base64-encoded image (alternative to prompt)
        image_url: Image URL (alternative to prompt)
        enable_pbr: Enable PBR material generation
        face_count: Target face count (3000-1500000)
        model: Model version ("3.0" or "3.1")
        poll_interval: Seconds between status checks
        max_wait: Maximum seconds to wait before timeout

    Returns:
        dict with keys: status, files (list of {type, url}), error
    """
    client = _get_client()

    # Submit job
    req = models.SubmitHunyuanTo3DProJobRequest()
    params = {"Model": model, "EnablePBR": enable_pbr, "FaceCount": face_count}

    if prompt:
        params["Prompt"] = prompt
    elif image_base64:
        params["ImageBase64"] = image_base64
    elif image_url:
        params["ImageUrl"] = image_url
    else:
        return {"status": "error", "error": "Provide either prompt, image_base64, or image_url"}

    req.from_json_string(json.dumps(params))
    submit_resp = client.SubmitHunyuanTo3DProJob(req)
    job_id = submit_resp.JobId

    # Poll for result
    start_time = time.time()
    while time.time() - start_time < max_wait:
        time.sleep(poll_interval)

        query_req = models.QueryHunyuanTo3DProJobRequest()
        query_req.from_json_string(json.dumps({"JobId": job_id}))
        query_resp = client.QueryHunyuanTo3DProJob(query_req)

        status = query_resp.Status

        if status == "DONE":
            files = []
            if query_resp.ResultFile3Ds:
                for f in query_resp.ResultFile3Ds:
                    files.append({"type": f.Type, "url": f.Url})
            return {"status": "done", "job_id": job_id, "files": files}

        elif status == "FAIL":
            return {
                "status": "failed",
                "job_id": job_id,
                "error": f"{query_resp.ErrorCode}: {query_resp.ErrorMessage}"
            }

    return {"status": "timeout", "job_id": job_id, "error": f"Exceeded {max_wait}s wait time"}


def download_result(url: str, filename: str = "") -> str:
    """Download a result file from URL to temp directory. Returns local file path."""
    import urllib.request

    TEMP_DIR.mkdir(exist_ok=True)

    if not filename:
        # Extract filename from URL or generate one
        from urllib.parse import urlparse
        parsed = urlparse(url)
        filename = os.path.basename(parsed.path) or "model.obj"

    local_path = TEMP_DIR / filename
    urllib.request.urlretrieve(url, str(local_path))
    return str(local_path).replace("\\", "/")
