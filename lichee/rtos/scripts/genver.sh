version_path=./include/generated/version.h

echo \#define SDK_UTS_VERSION    \"`date -R`\" >> $version_path
echo \#define SDK_COMPILE_TIME   \"`date +%T`\" >> $version_path
