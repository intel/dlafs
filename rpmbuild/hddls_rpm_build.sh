#!/bin/bash

RPMBUILD_PATH=$HOME/rpmbuild
PROJECTS_PATH=$(pwd)/..
SHELL_PATH=$(pwd)
RPM_SRC_PATH=$PROJECTS_PATH

WORK_VERSION="$1"
WORK_WEEK="$2"

if [[ "$WORK_VERSION" == "" ]];then
    echo -e "\033[1;31mPlease input work version\033[0m"
    exit 0
fi

if [[ "$WORK_WEEK" == "" ]];then
    echo -e "\033[1;31mPlease input work week num\033[0m"
    exit 0
fi

VERSION_NAME="$WORK_VERSION"
RELEASE_NAME="$WORK_WEEK"
SED_CMD_VERSION="s/Version:.*/Version:""$VERSION_NAME""/g"
SED_CMD_RELEASE="s/Release:.*/Release:""$RELEASE_NAME""/g"

echo "============================Form New Release Name==========================="
sed -i "$SED_CMD_VERSION" "$SHELL_PATH"/hddls_rpm_template.spec
echo -e "\033[1;33m`cat "$SHELL_PATH"/hddls_rpm_template.spec | grep 'Version:'`\033[0m"

sed -i "$SED_CMD_RELEASE" "$SHELL_PATH"/hddls_rpm_template.spec
echo -e "\033[1;33m`cat "$SHELL_PATH"/hddls_rpm_template.spec | grep 'Release:'`\033[0m"

echo "============================Making rpm package=============================="

#cd $RPM_SRC_PATH
#if [ $? -ne 0 ]; then
#echo "=========================directory 'output' not exist======================="
#exit 1;
#fi

if [ -f rpm_src_dir ]; then
    rm -rf rpm_src_dir
fi
mkdir rpm_src_dir

if [ $? -eq 0 ]; then
echo "=======================created directory 'rpm_src_dir'======================"
fi

mkdir -p rpm_src_dir/usr/lib/x86_64-linux-gnu/gstreamer-1.0
cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer-media-SDK/build/lib/release/* rpm_src_dir/usr/lib/x86_64-linux-gnu/gstreamer-1.0/.
cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/libgstcvdlfilter.so rpm_src_dir/usr/lib/x86_64-linux-gnu/gstreamer-1.0/.
#ln -sf rpm_src_dir/usr/lib/x86_64-linux-gnu/gstreamer-1.0/libgstcvdlfilter.so rpm_src_dir/usr/lib/x86_64-linux-gnu/libgstcvdlfilter.so
mkdir -p rpm_src_dir/usr/local/bin
cp -raf $PROJECTS_PATH/gstreamer_pipeline/hddlspipes rpm_src_dir/usr/local/bin/.
cp -raf $PROJECTS_PATH/gstreamer_pipeline/registeralgo rpm_src_dir/usr/local/bin/.
cp -raf $PROJECTS_PATH/gstreamer_pipeline/hddlspipestest rpm_src_dir/usr/local/bin/.
chmod a+x hddls_prepare.sh
chmod a+x hddls_pre_install.sh
cp -raf hddls_prepare.sh rpm_src_dir/usr/local/bin/.
cp -raf hddls_pre_install.sh rpm_src_dir/usr/local/bin/.

mkdir -p rpm_src_dir/usr/lib/x86_64-linux-gnu/libgstcvdl
cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/ocl/kernels rpm_src_dir/usr/lib/x86_64-linux-gnu/libgstcvdl/.
#cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/models rpm_src_dir/usr/lib/x86_64-linux-gnu/libgstcvdl/.
cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/resources/* rpm_src_dir/usr/lib/x86_64-linux-gnu/libgstcvdl/.
mkdir -p rpm_src_dir/usr/include/gstcvdl
#cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/ocl/oclcommon.h rpm_src_dir/usr/include/gstcvdl/.
cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/algo/exinferdata.h rpm_src_dir/usr/include/gstcvdl/.
cp -raf $PROJECTS_PATH/gstreamer_plugins/gstreamer_plugin_openVINO/gst-libs/algo/exinferenceparser.h rpm_src_dir/usr/include/gstcvdl/.

if [ $? -eq 0 ]; then 
echo "====================copied files to directory 'rpm_src_dir'================="
fi

cd rpm_src_dir
tar -cvzf Intel_Movidius_MyriadX_HDDL-S_Linux.tar.gz usr/

if [ $? -eq 0 ]; then
echo "===========================compressed directories==========================="
fi

if [ ! -d $RPMBUILD_PATH/SOURCES ]; then
    mkdir -p $RPMBUILD_PATH/SOURCES
    mkdir -p $RPMBUILD_PATH/SPECS
fi

mv Intel_Movidius_MyriadX_HDDL-S_Linux.tar.gz $RPMBUILD_PATH/SOURCES/ -f

cd ..
rm -rf rpm_src_dir/

cd $SHELL_PATH
cp hddls_rpm_template.spec $RPMBUILD_PATH/SPECS/ -f

cd $RPMBUILD_PATH/SPECS
rpmbuild -ba hddls_rpm_template.spec
