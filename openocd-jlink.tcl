source [find interface/jlink.cfg]
transport select swd

source [find target/infineon/psoc4.cfg]
${_TARGETNAME} configure -rtos auto -rtos-wipe-on-reset-halt 1

adapter speed 2000
