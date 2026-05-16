#!/bin/bash
# Bump the project version in CMakeLists.txt and package.sh Info.plist.
# Usage: ./bump-version.sh <major.minor.patch>
# Example: ./bump-version.sh 0.2.0

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 0.2.0"
    # Show current version
    CURRENT=$(grep "project(stepmaniax-gif-maker VERSION" CMakeLists.txt | sed 's/.*VERSION \([^ ]*\).*/\1/')
    echo "Current version: $CURRENT"
    exit 1
fi

VERSION="$1"

# Validate format
if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
    echo "Error: Version must be in format major.minor.patch (e.g., 0.2.0)"
    exit 1
fi

echo "Bumping version to $VERSION"

# CMakeLists.txt
sed -i "s/project(stepmaniax-gif-maker VERSION [0-9]*\.[0-9]*\.[0-9]*/project(stepmaniax-gif-maker VERSION $VERSION/" CMakeLists.txt

# package.sh Info.plist
sed -i "s/<string>[0-9]*\.[0-9]*\.[0-9]*<\/string>/<string>$VERSION<\/string>/g" package.sh

echo "Updated:"
echo "  CMakeLists.txt"
echo "  package.sh"
echo ""
echo "Next steps:"
echo "  git add -A && git commit -m \"Bump to v$VERSION\""
echo "  git tag v$VERSION"
echo "  git push && git push --tags"
