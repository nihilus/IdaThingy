Introduction
==============

Sometimes a small and simple tool can make a difference even if it just saves you a couple of minutes every day, or makes things look little cooler.
IdaThingy is a very simple plugin that allows you to send IDA to the tray and do its job in the background then you will get notified when IDA is done.
Because of its simplicity and utility Thingy is going to be in the arsenal of every reverse enginner.

       -- Thingy: Once you start, you can't stop! ;)

Usage
==========

Windows -> Minimize to tray: simple minimizes IDA to the tray.
Windows -> Background to tray: lowers IDA's process priority to below normal and then sends IDA to the tray area.
File -> Backup database: creates a backup of you currently opened database under the name "file_mm-dd-yy-hh-mm-ss.idb"

Installing
============
You may simple copy the plugin to IDA's folder and/or you may also configure it in plugins.cfg.

        thingy_to_tray                   thingy      Alt-1 1                    
        thingy_to_bkg_tray               thingy      Alt-2 2                    


Hope you find it useful.


History
==========
05/14/2007 - Now IDA gets the focus after it is restored from tray

Copyright
============

IdaThingy (c) <lallousx86@yahoo.com> / http://lgwm.org/

Find more plugins from the same author:
 - JPlus: Go from function start to end with ease!
 - VbDllFunctionCall: Easily reconstruct VB6 external dll calls!
 - StDump: Copy/Paste IDA data types from clipboard or to C/Pascal files!
 - DbgPlus: Command Line interface for IDA debug plugins!

// Barty