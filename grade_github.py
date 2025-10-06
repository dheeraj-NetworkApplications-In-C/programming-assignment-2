import subprocess
import re
import json
import sys

def run_grading_script(script_path):
    try:
        result = subprocess.run(
            ["bash", script_path],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
        return result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        print("Script failed with error code:", e.returncode)
        print("Error output:\n", e.stderr)
        sys.exit(1)

def parse_output(output):
    ftp_features = {
        "ls contains": "ls",
        "Hashes match.": "get",
        "'foo1' not present after delete": "delete",
        "Hashes match after put.": "put",
        "Server exited": "exit"
    }
    reliability_marker = "Hashes match under netem"

    ftp_total = 75
    reliability_total = 25
    ftp_score = 0
    reliability_score = 0
    ftp_per_feature = ftp_total / len(ftp_features)
    ftp_results = {}

    for marker, name in ftp_features.items():
        if marker in output:
            ftp_score += ftp_per_feature
            ftp_results[name] = ftp_per_feature
        else:
            ftp_results[name] = 0

    if reliability_marker in output:
        reliability_score = reliability_total

    total_score = ftp_score + reliability_score

    return {
        "Basic FTP Features": round(ftp_score),
        "Reliable Transfer": round(reliability_score),
        "Total Score": round(total_score),
        "Details": ftp_results
    }

def determine_status(score):
    if score == 100:
        return "PASS ✅", 0
    elif score >= 75:
        return "WARNING ⚠️", 0
    else:
        return "FAIL ❌", 1

def main():
    
    print("Running grading script...")
    script_path = "./grade_py.sh"
    stdout, stderr = run_grading_script(script_path)
    scores = parse_output(stdout)

    status, exit_code = determine_status(scores["Total Score"])

    print("\n=== Grading Summary ===")
    print(f"Status: {status}")
    print(f"Basic FTP Features: {scores['Basic FTP Features']} / 75")
    print(f"Reliable Transfer under netem packet loss: {scores['Reliable Transfer']} / 25")
    print(f"Total Score: {scores['Total Score']} / 100")
    print("\nDetails:")
    for feature, score in scores["Details"].items():
        print(f"- {feature}: {score} points")

    # Write to file for GitHub summary
    with open("parsed_scores.json", "w") as f:
        json.dump(scores, f, indent=2)

    sys.exit(exit_code)

if __name__ == "__main__":
    main()
