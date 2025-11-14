Here(utils) are some scripts for user to collect useful informations.

1. get-version
	1.1 User may use it to collect version number of toolchains. It will generate 'version.log'.
	1.2 How to use?
		./get-version

2. analyze.sh
	2.1 User may use it to collect the code size informations.
	2.2 How to use?
		2.2.1 User need to put all his .o files(that will be linked) in the same folder.
		2.2.2 ./analyze.sh the_elf_file

	2.3 How to config?
		2.3.1 The toolchain path is needed to be configured. You have to set your TOOLPATH.
		2.3.2 You can specify the libraries. You can mark the LIBRARIES which you don't 
		      actually linked. You can know the library that you linked by add 
        	      "-Wl,-Map,link.map" to your LDFLAG.

3. nds-imix-inst
	3.1 User may use it to collect instructions statistics.
	3.2 How to use?
		./nds-imix-inst objdump_file

4. nds_ldsag
	Please refer to user manual.
