#!/usr/bin/env python3
"""
Branch name validator for EdgeHermes
Works on all platforms (Windows, macOS, Linux)
"""
import re
import subprocess
import sys


def get_current_branch():
    """Get the current git branch name."""
    try:
        result = subprocess.run(
            ["git", "rev-parse", "--abbrev-ref", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return None


def is_protected_branch(branch_name):
    """Check if branch is protected (doesn't need validation)."""
    protected_patterns = [
        r"^main$",
        r"^master$",
        r"^develop$",
        r"^release/.+",
        r"^hotfix/.+",
        r"^HEAD$",  # Detached HEAD
    ]
    return any(re.match(pattern, branch_name) for pattern in protected_patterns)


def validate_branch_name(branch_name):
    """
    Validate branch name against the required format.
    Format: <type>-<description> or <type>/<description>
    """
    # Pattern: type followed by - or / followed by lowercase alphanumeric
    valid_types = [
        "feat",
        "fix",
        "docs",
        "style",
        "refactor",
        "perf",
        "test",
        "build",
        "ci",
        "chore",
    ]
    pattern = rf"^({'|'.join(valid_types)})([-/])[a-z0-9_-]+$"
    return re.match(pattern, branch_name) is not None


def print_error_message(branch_name):
    """Print helpful error message for invalid branch names."""
    error_msg = f"""
{'='*70}
              ‚ù?INVALID BRANCH NAME
{'='*70}

Branch: {branch_name}

Branch names must follow this format:
  <type>-<description>  or  <type>/<description>

Valid types:
  ‚Ä?feat      - New feature
  ‚Ä?fix       - Bug fix
  ‚Ä?docs      - Documentation changes
  ‚Ä?style     - Code style changes
  ‚Ä?refactor  - Code refactoring
  ‚Ä?perf      - Performance improvements
  ‚Ä?test      - Test changes
  ‚Ä?build     - Build system changes
  ‚Ä?ci        - CI/CD changes
  ‚Ä?chore     - Other changes

‚ú?Valid examples:
  feat-buffer-pooling
  fix-windows-dll-exports
  docs-update-readme
  refactor/simplify-tensor-allocation
  ci-add-coverage-reporting

‚ù?Current branch: {branch_name}

To fix this, rename your branch:
  git branch -m {branch_name} <type>-<proper-description>

Or delete and recreate:
  git checkout main
  git branch -D {branch_name}
  git checkout -b <type>-<proper-description>

{'='*70}
"""
    print(error_msg, file=sys.stderr)


def main():
    """Main entry point for branch name validation."""
    branch_name = get_current_branch()

    if not branch_name:
        # Can't determine branch, skip validation
        sys.exit(0)

    # Skip protected branches
    if is_protected_branch(branch_name):
        sys.exit(0)

    # Validate branch name
    if not validate_branch_name(branch_name):
        print_error_message(branch_name)
        sys.exit(1)

    # Branch name is valid
    print(f"‚ú?Branch name '{branch_name}' is valid")
    sys.exit(0)


if __name__ == "__main__":
    main()




