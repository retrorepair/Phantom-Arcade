#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade - GitHub Private Repository Push Helper
# Usage: ./push_to_github.sh <GITHUB_PERSONAL_ACCESS_TOKEN> [REPO_NAME]
# =============================================================================

TOKEN="$1"
REPO_NAME="${2:-phantom-arcade-bridge}"
USERNAME="joelwhybrow"

if [ -z "$TOKEN" ]; then
    echo "Usage: ./push_to_github.sh <GITHUB_PERSONAL_ACCESS_TOKEN> [REPO_NAME]"
    echo "Example: ./push_to_github.sh ghp_xxxxxx phantom-arcade-bridge"
    exit 1
fi

echo "[+] Creating private GitHub repository '$REPO_NAME'..."

# Create private repository via GitHub REST API
RESPONSE=$(curl -s -w "\n%{http_code}" -X POST \
  -H "Authorization: token $TOKEN" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/user/repos \
  -d "{\"name\":\"$REPO_NAME\",\"private\":true,\"description\":\"Groovy_MiSTer Client-Server Bridge and Arcade Launcher Suite for 15kHz CRT gaming.\"}")

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" -eq 201 ] || [ "$HTTP_CODE" -eq 422 ]; then
    echo "[+] Repository created or already exists on GitHub."
else
    echo "[-] Failed to create repo via GitHub API (HTTP $HTTP_CODE):"
    echo "$BODY"
fi

echo "[+] Setting up git remote and pushing to master..."
git branch -M main 2>/dev/null || git branch -M master
git remote remove origin 2>/dev/null
git remote add origin "https://${TOKEN}@github.com/${USERNAME}/${REPO_NAME}.git"

git push -u origin HEAD --force

echo ""
echo "=========================================================="
echo " SUCCESS: Pushed to https://github.com/${USERNAME}/${REPO_NAME}"
echo "=========================================================="
