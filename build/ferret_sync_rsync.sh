
mkdir -p "$2"
echo "rsync call for source $1 target $2"
rsync -vxa  "$1"  "$2"

exit 0
