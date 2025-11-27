# NovaLLM Development Setup

Quick guide for new contributors.

## One-Time Setup (After Cloning)

Run this single command to set up all development hooks:

```bash
pip install pre-commit && pre-commit install --hook-type commit-msg --hook-type pre-commit --hook-type post-checkout
```

**What this does:**
- Installs pre-commit (if not already installed)
- Enables automatic validation on every commit
- Enables branch name validation
- Enables code quality checks

## Naming Conventions

### Branch Names

Format: `<type>-<description>`

**Valid types:** feat, fix, docs, style, refactor, perf, test, build, ci, chore

**Examples:**
```bash
git checkout -b feat-add-buffer-pooling     ✅
git checkout -b fix-memory-leak             ✅
git checkout -b docs-update-readme          ✅
git checkout -b my-branch                   ❌ (no type prefix)
```

### Commit Messages

Format: `<type>(<scope>): <subject>`

**Examples:**
```bash
git commit -m "feat(memory): add buffer pooling"     ✅
git commit -m "fix(build): correct DLL exports"      ✅
git commit -m "docs(readme): update setup guide"     ✅
git commit -m "update code"                          ❌ (no type)
```

## What Happens Automatically

After setup, the hooks will:

1. **On `git checkout -b new-branch`:**
   - ✅ Validate branch name format
   - ❌ Reject invalid branch names with helpful error

2. **On `git commit`:**
   - ✅ Format C++ code with clang-format
   - ✅ Check for trailing whitespace, large files, etc.
   - ✅ Validate commit message format
   - ❌ Reject invalid commits with helpful error

## Cross-Platform Support

Works on:
- ✅ macOS (zsh, bash)
- ✅ Linux (bash, zsh, sh)
- ✅ Windows (Git Bash, PowerShell, WSL)

Requirements:
- Python 3.6+ (comes with most systems)
- Git (already have it if you cloned the repo)
- pip (to install pre-commit)

## Troubleshooting

### "pre-commit: command not found"
```bash
pip install pre-commit
# or
pip3 install pre-commit
```

### "Permission denied" on Linux/macOS
```bash
chmod +x .githooks/*.py
chmod +x .githooks/*.sh
```

### Need to bypass hooks temporarily?
```bash
git commit --no-verify -m "WIP: work in progress"
```

**⚠️ Warning:** Bypassed commits will still be checked by CI!

## Full Documentation

See [.pre-commit-setup.md](.pre-commit-setup.md) for complete documentation.

## Build Instructions

See [README.md](README.md) for build and development instructions.
