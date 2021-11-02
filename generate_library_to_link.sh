target="/Users/user/Projects/intellij/rplugin/rwrapper/vcpkg/installed/x64-osx/lib"
lb_list=""
for f in "$target"/*.a
do
    filename=$(basename $f)
    shortname="${filename%.*}"
    echo "add_library($shortname STATIC IMPORTED)"
    echo "set_target_properties($shortname PROPERTIES IMPORTED_LOCATION \"\${GRPC_DIST}/lib/$shortname\${CMAKE_STATIC_LIBRARY_SUFFIX}\")"
    lb_list="$lb_list $shortname"
done

echo "list"
echo $lb_list

