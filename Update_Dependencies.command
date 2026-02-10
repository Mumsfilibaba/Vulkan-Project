DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
cd "${DIR}"

git submodule sync --recursive
git submodule update --init --recursive
git submodule update --remote --recursive