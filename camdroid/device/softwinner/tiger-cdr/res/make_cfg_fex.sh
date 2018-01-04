#! /bin/sh
CFG_PATH="/pack/chips/sun8iw8p1/configs/CDR/cfg.fex"
DEST=$LICHEE_TOOLS_DIR$CFG_PATH

echo "./mkfs.jffs2 -d ./data -o cfg.fex"
#-p total size
./mkfs.jffs2 -d ./cfg -p 0x80000 -o cfg.fex

echo "move cfg.fex to $DEST"
mv cfg.fex $DEST

