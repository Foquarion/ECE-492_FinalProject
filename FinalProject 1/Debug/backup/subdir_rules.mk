################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
backup/(backup)timers_b0.obj: ../backup/(backup)timers_b0.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --use_hw_mpy=F5 --include_path="C:/ti/ccs1281/ccs/ccs_base/msp430/include" --include_path="C:/Users/Dallas.Owens/Documents/!NC State/!Fall '25/ECE 306/Projects/Project 10/Project 10_a/Project 10_a" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --advice:hw_config=all --define=__MSP430FR2355__ --define=_FRWP_ENABLE --define=_INFO_FRWP_ENABLE -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="backup/(backup)timers_b0.d_raw" --obj_directory="backup" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

backup/(backup3)interrupts_UART.obj: ../backup/(backup3)interrupts_UART.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --use_hw_mpy=F5 --include_path="C:/ti/ccs1281/ccs/ccs_base/msp430/include" --include_path="C:/Users/Dallas.Owens/Documents/!NC State/!Fall '25/ECE 306/Projects/Project 10/Project 10_a/Project 10_a" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --advice:hw_config=all --define=__MSP430FR2355__ --define=_FRWP_ENABLE --define=_INFO_FRWP_ENABLE -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="backup/(backup3)interrupts_UART.d_raw" --obj_directory="backup" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

backup/(backup4)UART.obj: ../backup/(backup4)UART.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --use_hw_mpy=F5 --include_path="C:/ti/ccs1281/ccs/ccs_base/msp430/include" --include_path="C:/Users/Dallas.Owens/Documents/!NC State/!Fall '25/ECE 306/Projects/Project 10/Project 10_a/Project 10_a" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --advice:hw_config=all --define=__MSP430FR2355__ --define=_FRWP_ENABLE --define=_INFO_FRWP_ENABLE -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="backup/(backup4)UART.d_raw" --obj_directory="backup" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

backup/(backup4)interrupts_timers.obj: ../backup/(backup4)interrupts_timers.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/bin/cl430" -vmspx --use_hw_mpy=F5 --include_path="C:/ti/ccs1281/ccs/ccs_base/msp430/include" --include_path="C:/Users/Dallas.Owens/Documents/!NC State/!Fall '25/ECE 306/Projects/Project 10/Project 10_a/Project 10_a" --include_path="C:/ti/ccs1281/ccs/tools/compiler/ti-cgt-msp430_21.6.1.LTS/include" --advice:power=all --advice:hw_config=all --define=__MSP430FR2355__ --define=_FRWP_ENABLE --define=_INFO_FRWP_ENABLE -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="backup/(backup4)interrupts_timers.d_raw" --obj_directory="backup" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


