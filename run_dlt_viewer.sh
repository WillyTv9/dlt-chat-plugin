#!/bin/bash
# Start DLT Viewer with mock DLT file loaded

cd /home/user/Desktop/dlt-viewer-plugin/dlt-viewer/build/bin
export LD_LIBRARY_PATH=.
./dlt-viewer &
sleep 3
echo ""
echo "============================================"
echo " DLT Viewer Started"
echo "============================================"
echo ""
echo "To test the plugin:"
echo "1. Settings -> Plugin Settings -> Enable 'DLT Log Assistant'"
echo "2. File -> Open DLT File"
echo "3. Select: ../plugin/dltchatplugin/mock_dlt_log.dlt"
echo "4. View -> Panels -> DLT Log Assistant"
echo "5. Try queries:"
echo "   - mostra errori"
echo "   - riassumi"
echo "   - timeout"
echo "   - CAN"
echo ""