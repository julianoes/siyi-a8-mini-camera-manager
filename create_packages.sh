#!/usr/bin/env bash
#
# Script copied from:
# https://github.com/mavlink/MAVSDK/blob/main/tools/create_packages.sh

set -e

if [ "$#" -ne 4 ]; then
  echo "Error: missing path to the 'install' directory (i.e. what was specified as -DCMAKE_INSTALL_PREFIX)" >&2
  echo "       or output path (where the package will be generated)!" >&2
  echo "Usage: $0 <path/to/install> <output/path> <architecture> <name>" >&2
  exit 1
fi

if ! command -v fpm &> /dev/null
then
    echo "fpm could not be found" >&2
    echo ""
    echo "To install:" >&2
    echo "sudo apt install rubygems" >&2
    echo "sudo gem install fpm" >&2
    exit 1
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
install_dir=$1
output_dir=$2
arch="$3"
name="$4"

# The package architecture needs to be armhf, at least for RPi.
if [ "$arch" = "armv6" ] || [ "$arch" = "armv7" ]; then
    package_arch="armhf"
else
    package_arch=$arch
fi

working_dir=$(mktemp -d)
mkdir -p ${working_dir}/install
cp -r ${install_dir} ${working_dir}/install/usr

# We need the relative path of all files.
library_files=$(find ${working_dir}/install -type f | cut -sd / -f 5-)
echo "Files to be packaged:"
for file in ${library_files}; do
    echo "  '$file'"
done

# This creates a version such as "v1.2.3-5-g123abc".
git_describe=$(git describe --always --tags)
# We want to extract 1.2.3 from it.
version=$(echo ${git_describe} | sed 's/v\([0-9]*\.[0-9]*\.[0-9]*\).*$/\1/')

echo "Found version: '${version}' from '${git_describe}'."

common_args="--chdir ${working_dir}/install \
             --input-type dir  \
             --name ${name} \
             --version ${version} \
             --maintainer julian@oes.ch \
             --url https://github.com/julianoes/siyi-a8-mini-camera-manager \
             --license Apache-2.0-license \
             --force \
             -a ${package_arch}"

echo "common_args: ${common_args}"

if cat /etc/os-release | grep -e 'Ubuntu' -e 'Mint'
then
    echo "Building Ubuntu DEB package"
    fpm ${common_args} \
        --output-type deb \
        --deb-no-default-config-files \
        ${library_files}

    dist_version=$(cat /etc/os-release | grep VERSION_ID | sed 's/[^0-9.]*//g')

    for file in *_${package_arch}.deb
    do
        mv -v "${file}" "${output_dir}/${file%_${package_arch}.deb}_ubuntu${dist_version}_${arch}.deb"
    done

elif cat /etc/os-release | grep 'Debian'
then
    echo "Building Debian DEB package"

    fpm ${common_args} \
        --output-type deb \
        --deb-no-default-config-files \
        --deb-pre-depends libgstrtspserver-1.0-0 \
        ${library_files}

    dist_version=$(cat /etc/os-release | grep VERSION_ID | sed 's/[^0-9.]*//g')

    for file in *_${package_arch}.deb
    do
        mv -v "${file}" "${output_dir}/${file%_${package_arch}.deb}_debian${dist_version}_${arch}.deb"
    done

elif cat /etc/os-release | grep 'Fedora'
then
    echo "Building RPM package"
    fpm ${common_args} \
        --output-type rpm \
        --rpm-rpmbuild-define "_build_id_links none" \
        ${library_files}

    dist_version=$(cat /etc/os-release | grep VERSION_ID | sed 's/[^0-9]*//g')

    for file in *.${package_arch}.rpm
    do
        mv -v "${file}" "${output_dir}/${file%.${package_arch}.rpm}.fc${dist_version}-${arch}.rpm"
    done
fi

echo "Done."
