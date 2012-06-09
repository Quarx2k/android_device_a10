# This script is included in releasetools/addons
# It is the final build step (after OTA package)

echo "addons.sh: $1"

if [ -z "$1" ]; then
	echo "addons.sh: error ! no target specified"
	exit 1;
fi


