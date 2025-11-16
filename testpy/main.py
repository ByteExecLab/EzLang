import subprocess
from pathlib import Path
import textwrap

# ANSI colors
GREEN = "\033[92m"
RED = "\033[91m"
YELLOW = "\033[93m"
RESET = "\033[0m"

exe_file = Path("../ezlang")
tests_dir = Path("../tests")

results = []

def short_diff(expected, actual, limit=200):
    """Return a short diff block for display."""
    exp = expected.strip().splitlines()
    act = actual.strip().splitlines()
    diff = []

    for i, (e, a) in enumerate(zip(exp, act)):
        if e != a:
            diff.append(f"Line {i+1}:\n  expected: {e}\n  got:      {a}")
            break

    if not diff:
        return "Outputs differ (length or trailing newline mismatch)."

    return "\n".join(diff[:limit])


for ez_path in tests_dir.glob("*.ez"):
    # Expect: file.ez.out
    expected_path = ez_path.with_suffix(ez_path.suffix + ".out")

    if not expected_path.exists():
        print(f"{YELLOW}MISSING{RESET} {ez_path.name} → {expected_path.name} not found")
        results.append(("MISSING", ez_path.name))
        continue

    expected_output = expected_path.read_text(encoding="utf-8", errors="ignore")

    result = subprocess.run(
        [str(exe_file), str(ez_path)],
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        print(f"{RED}ERROR{RESET}   {ez_path.name} (exit code {result.returncode})")
        if result.stderr:
            print(textwrap.indent(result.stderr.strip(), "    "))
        results.append(("ERROR", ez_path.name))
        continue

    if result.stdout == expected_output:
        print(f"{GREEN}OK{RESET}      {ez_path.name}")
        results.append(("OK", ez_path.name))
    else:
        print(f"{RED}FAIL{RESET}    {ez_path.name}")
        print(textwrap.indent(short_diff(expected_output, result.stdout), "    "))
        results.append(("FAIL", ez_path.name))


# Summary
print("\n────────── SUMMARY ──────────")
total = len(results)
ok = sum(1 for r in results if r[0] == "OK")
fail = sum(1 for r in results if r[0] == "FAIL")
missing = sum(1 for r in results if r[0] == "MISSING")
errors = sum(1 for r in results if r[0] == "ERROR")

print(f"Total: {total}")
print(f"{GREEN}OK: {ok}{RESET}")
print(f"{RED}FAILED: {fail}{RESET}")
print(f"{YELLOW}MISSING: {missing}{RESET}")
print(f"{RED}ERRORS: {errors}{RESET}")

if fail or errors:
    print("\nSome tests did not pass.")
else:
    print(f"{GREEN}All tests passed! 🎉{RESET}")
