#!/usr/bin/env python3
"""Keep the POC's default rewards aligned with Avida-ED 4's nine-task profile."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "NOT": 2.0,
    "NAND": 2.0,
    "AND": 4.0,
    "OR_NOT": 4.0,
    "OR": 8.0,
    "AND_NOT": 8.0,
    "NOR": 16.0,
    "XOR": 16.0,
    "EQU": 32.0,
}


def config_rewards(text: str) -> dict[str, float]:
    return {
        task: float(value)
        for task, value in re.findall(
            r"^Reaction\s+(\w+)\s+metabolic_mult\s+mult\s+([0-9.]+)\s+1\s*$",
            text,
            re.MULTILINE,
        )
    }


web_config = (ROOT / "config/Avida-web.cfg").read_text(encoding="utf-8")
module_source = (ROOT / "source/Modules/EnvironmentLogic.hpp").read_text(encoding="utf-8")
assert config_rewards(web_config) == EXPECTED, "web config differs from the Avida-ED 4 reward profile"
module_rewards = {
    task: float(value)
    for task, value in re.findall(
        r'"Reaction\s+(\w+)\s+metabolic_mult\s+mult\s+([0-9.]+)\s+1\\n"',
        module_source,
    )
}
assert module_rewards == EXPECTED, "serialized default config differs from the Avida-ED 4 reward profile"
assert "ECHO" not in config_rewards(web_config)
print("validated nine default reward tasks and strengths in config and module defaults")
