#!/usr/bin/env bash
# =============================================================================
# Phantom Arcade - GitHub Repository Push Helper
# Usage: ./push_to_github.sh <GITHUB_PERSONAL_ACCESS_TOKEN> [REPO_NAME]
# =============================================================================

TOKEN="$1"
REPO_NAME="${2:-Phantom-Arcade}"
USERNAME="retrorepair"

if [ -z "$TOKEN" ]; then
    echo "Usage: ./push_to_github.sh <GITHUB_PERSONAL_ACCESS_TOKEN> [REPO_NAME]"
    echo "Example: ./push_to_github.sh ghp_xxxxxx Phantom-Arcade"
    exit 1
fi

echo "[+] Pushing to https://github.com/${USERNAME}/${REPO_NAME}..."

# Test repository existence via GitHub REST API
RESPONSE=$(curl -s -w "\n%{http_code}" -X GET \
  -H "Authorization: token $TOKEN" \
  -H "Accept: application/vnd.github.v3+json" \
  https://api.github.com/repos/${USERNAME}/${REPO_NAME})

HTTP_CODE=$(echo "$RESPONSE" | tail -n1)
BODY=$(echo "$RESPONSE" | sed '$d')

if [ "$HTTP_CODE" -eq 200 ]; then
    echo "[+] Repository found on GitHub (HTTP 200)."
elif [ "$HTTP_CODE" -eq 404 ]; then
    echo "[+] Repository not found on user account. Attempting to create repository '${REPO_NAME}'..."
    CREATE_RESP=$(curl -s -X POST \
      -H "Authorization: token $TOKEN" \
      -H "Accept: application/vnd.github.v3+json" \
      https://api.github.com/user/repos \
      -d "{\"name\":\"$REPO_NAME\",\"private\":false,\"description\":\"Groovy_MiSTer Client-Server Bridge and Arcade Launcher Suite for 15kHz CRT gaming.\"}")
fi

echo "[+] Setting up git remote and pushing to main..."
git branch -M main 2>/dev/null || git branch -M master
git remote remove origin 2>/dev/null
git remote add origin "https://${TOKEN}@github.com/${USERNAME}/${REPO_NAME}.git"

git push -u origin HEAD --force

echo ""
echo "=========================================================="
echo " SUCCESS: Pushed to https://github.com/${USERNAME}/${REPO_NAME}"
echo "=========================================================="
echo ""
echo "NOTE ON REPO VISIBILITY:"
echo " - If this repository is PUBLIC:"
echo "   MiSTer can pull scripts directly with 0 authentication:"
echo "   curl -k -sSL https://raw.githubusercontent.com/${USERNAME}/${REPO_NAME}/main/mister_client/install_mister.sh | bash"
echo ""
echo " - If this repository is PRIVATE:"
echo "   GitHub will return 404 to unauthenticated curl requests."
echo "   Either set the repo to Public under Settings -> Danger Zone,"
echo "   OR copy mister_client/Phantom_Arcade.sh to MiSTer via SCP:"
echo "   scp mister_client/Phantom_Arcade.sh root@<MISTER_IP>:/media/fat/Scripts/"
echo "=========================================================="
