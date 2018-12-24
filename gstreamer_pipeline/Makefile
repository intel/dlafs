.PHONY: all clean install

all:
	@make -C hddls_pipe_hal
	@cp hddls_pipe_hal/src/hddlspipes .
	@cp hddls_pipe_hal/tools/registeralgo .
	@cp hddls_pipe_hal/test/hddlspipestest .
clean:
	@make clean -C hddls_pipe_hal
	@rm -rf hddlspipes registeralgo hddlspipestest
install:
	@make install -C hddls_pipe_hal
	@echo ""
	@echo "Success to install hddlspipes"
	@echo ""
uninstall:
	@make uninstall -C hddls_pipe_hal
	@echo "Success to uninstall hddlspipes"
	@echo ""
