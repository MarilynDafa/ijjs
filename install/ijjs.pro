TEMPLATE = aux

INSTALLER = installer

INPUT = $$PWD/config/config.xml $$PWD/packages
example.input = INPUT
example.output = $$INSTALLER
win32:example.commands = $$PWD/bin/binarycreator -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
macx:example.commands = /Users/zhoubo/Qt/Tools/QtInstallerFramework/3.2/bin/binarycreator -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
example.CONFIG += target_predeps no_link combine

QMAKE_EXTRA_COMPILERS += example

OTHER_FILES = README
