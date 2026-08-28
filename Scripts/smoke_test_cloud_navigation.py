"""End-to-end cloud smoke test for InterVerseSG navigation.

Runs without Unreal Engine. It verifies that a natural-language command sent to
InterVerse API is validated by InterVerse Builder and resolves to the expected
NAV anchor. Transient Render/Gemini failures are retried before the workflow is
marked as failed.
"""

from __future__ import annotations

import json
import sys
import time
import urllib.error
import urllib.request

API_URL = "https://interverse-api-yhqx.onrender.com/api/v1/assistant"
BUILDER_URL = "https://interverse-builder.onrender.com/api/v1/build/validate"
TEST_MESSAGE = "Llévame a la Escuela Graduada"
EXPECTED_ANCHOR = "NAV_EscuelaGraduada"
MAX_ATTEMPTS = 3
RETRY_DELAY_SECONDS = 5


def post_json(url: str, payload: dict) -> dict:
    body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
    request = urllib.request.Request(
        url,
        data=body,
        method="POST",
        headers={
            "Content-Type": "application/json",
            "User-Agent": "InterVerseSG-GitHubActions/1.1",
        },
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        return json.loads(response.read().decode("utf-8"))


def run_once() -> None:
    assistant = post_json(
        API_URL,
        {
            "message": TEST_MESSAGE,
            "context": "InterVerseSG automated cloud smoke test",
            "session_id": "github-actions-smoke-test",
        },
    )

    if assistant.get("action") != "navigate":
        raise AssertionError(f"Expected navigate, got: {assistant}")

    validated = post_json(BUILDER_URL, assistant)

    if validated.get("accepted") is not True:
        raise AssertionError(f"Builder rejected command: {validated}")
    if validated.get("action") != "navigate":
        raise AssertionError(f"Builder action is not navigate: {validated}")
    if validated.get("navigation_anchor") != EXPECTED_ANCHOR:
        raise AssertionError(
            f"Expected {EXPECTED_ANCHOR}, got {validated.get('navigation_anchor')}: {validated}"
        )


def main() -> int:
    last_error = None
    for attempt in range(1, MAX_ATTEMPTS + 1):
        try:
            run_once()
            print(f"PASS: natural-language command -> Gemini -> Builder (attempt {attempt})")
            print(f"PASS: {TEST_MESSAGE!r} -> {EXPECTED_ANCHOR}")
            return 0
        except (AssertionError, urllib.error.URLError, urllib.error.HTTPError, TimeoutError) as exc:
            last_error = exc
            print(f"WARN: cloud smoke attempt {attempt}/{MAX_ATTEMPTS} failed: {exc}", file=sys.stderr)
            if attempt < MAX_ATTEMPTS:
                time.sleep(RETRY_DELAY_SECONDS)

    raise AssertionError(f"Cloud smoke test failed after {MAX_ATTEMPTS} attempts: {last_error}")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AssertionError as exc:
        print(f"FAIL: {exc}", file=sys.stderr)
        raise SystemExit(1)
