SUMMARY = "A small image for my project."

#require recipes-core/images/core-image-minimal.bb
require recipes-sato/images/core-image-sato.bb

inherit extrausers
EXTRA_USERS_PARAMS = "usermod -P abcd root;"

IMAGE_ROOTFS_EXTRA_SPACE = "1000000"

IMAGE_FEATURES += "debug-tweaks ssh-server-openssh"

IMAGE_INSTALL += "\
libxcursor \
packagegroup-qt5-qtcreator-debug \
packagegroup-qt5-toolchain-target \
"
