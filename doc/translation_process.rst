_

Bean Cash Translation Notes
===========================

The Qt GUI can be easily translated into other languages. Here's how we
handle those translations.

Files and Folders
-----------------

Beancash-qt.pro
~~~~~~~~~~~~~~~

This file takes care of generating `.qm` files from `.ts` files. It is mostly
automated.

src/qt/beancash.qrc
~~~~~~~~~~~~~~~~~~~

This file must be updated whenever a new translation is added. Please note that
files must end with `.qm`, not `.ts`.
::

    <qresource prefix="/translations">
        <file alias="en">locale/beancash_en.qm</file>
        ...
    </qresource>

src/qt/locale/
~~~~~~~~~~~~~~

This directory contains all translations. Filenames must adhere to this format:
::

    beancash_xx_YY.ts or beancash_xx.ts

Source file
~~~~~~~~~~~

`src/qt/locale/beancash_en.ts` is treated in a special way. It is used as the
source for all other translations. Whenever a string in the code is changed
this file must be updated to reflect those changes. Usually, this can be
accomplished by running `lupdate` (included in the Qt SDK).

Translations for Bean Cash will be handled by our own hosted translation service:

https://translate.beancash.net 

(If services are not yet operational, check back latter.)

Please email if you are interested to help with translation:  support@beancash.org



