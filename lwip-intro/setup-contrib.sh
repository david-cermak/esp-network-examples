WGET_PATH="http://download.savannah.nongnu.org/releases/lwip"
CONTRIB="contrib-2.1.0"

wget --no-verbose ${WGET_PATH}/${CONTRIB}.zip
unzip -oq ${CONTRIB}.zip
rm ${CONTRIB}.zip
patch -s -p0 < lwip/test/${CONTRIB}.patch
cp ${CONTRIB}/examples/example_app/lwipcfg.h.example ${CONTRIB}/examples/example_app/lwipcfg.h
