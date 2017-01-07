# git://code.qt.io/yocto/meta-boot2qt.git
SOURCE_FOLDER="$PWD"
POKY_FOLDER="$HOME/poky"
export MACHINE=beaglebone
if [ ! -d "$POKY_FOLDER" ]; then
	git clone git://git.yoctoproject.org/poky $POKY_FOLDER
fi
if [ -d $POKY_FOLDER/meta-nsg ]; then
	rm -rf $POKY_FOLDER/meta-nsg
fi	
cp -r meta-nsg $POKY_FOLDER
if [ ! -d "$POKY_FOLDER/meta-qt5" ]; then
	git clone git://code.qt.io/yocto/meta-qt5.git $POKY_FOLDER/meta-qt5
fi
cd $POKY_FOLDER
git checkout -b morty origin/morty
source ./oe-init-build-env build-$MACHINE
cp $SOURCE_FOLDER/meta-nsg/conf/bblayers.conf $POKY_FOLDER/build-$MACHINE/conf
cp $SOURCE_FOLDER/meta-nsg/conf/local.conf $POKY_FOLDER/build-$MACHINE/conf
bitbake nsg-dev
bitbake nsg-dev -c populate_sdk
printenv
#bitbake meta-toolchain-qt5
#./tmp/deploy/sdk/poky-glibc-x86_64-nsg-dev-i586-toolchain-2.2.sh
#./tmp/deploy/sdk/poky-glibc-x86_64-nsg-dev-cortexa8hf-neon-toolchain-2.2.sh
#runqemu qemux86
#ssh root@192.168.7.2
#scp xxxx root@192.168.7.2:~

# remote debugging (target)
# target remote iplocahost:port

# debugging on host
# gdb 
# set auto-load safe-path /
# set sysroot $SDKTARGETSYSROOT
# target remote ip:port
# break main
# continue

#lsblk -d
#dd bs=4k if=b2qt-embedded-qt5-image-beaglebone.img of=/dev/sdf status=progress; sync
#pv -p b2qt-embedded-qt5-image-beaglebone.img | sudo dd bs=4k of=/dev/sdf
#dd if=b2qt-embedded-qt5-image-beaglebone.img | pv -p | sudo dd of=/dev/sdf
#find . -print | grep zlib
#grep -r IMAGE_TYPES .
#sudo dd bs=4k if=nsg-dev-beaglebone.wic of=/dev/sdf status=progress; sync