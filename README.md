----------------------------------------------------------------------------
Deep Learning Accelerator Framework for Stream (DLAFS) project
----------------------------------------------------------------------------

Deep Learning Accelerator Framework for Stream (DLAFS) is based on Gstreamer. Leveraging MediaSDK, OpenCL, OpenCV and OpenVINO, DLAFS is able to perform video codec, computer vision processing, deep learning processing (including but not limited to object detection and classification/recognition).

0. Compile and install
----------------------
	Refer to DLAFS_Release_Notes_2020R2.pdf for detailed steps.

1. Run a test case
------------------
	cd scripts/1.16.2
	./prepare_omz_resource.sh
	source setup_test
	cd ../test/1.16.2
	./uc4.sh
	Several JPEG file as well as metadata.json should be created.

2. Best known configuration
---------------------------
	CPU: Intel(R) Core (TM) i7-7700K CPU @ 4.20GHz
	OS:  Ubuntu 18.04.4
	Kernel: 5.3.0-28-generic
	OpenVINO: 2020.3.194
	OpenCL SDK: 2019.5.345
	Gstreamer: 1.16.2 (OpenVINO 2020.3.194)
	OpenCV: 4.3.0-openvino with Intel VA support
	JSON-C: f8c632f579c71012f9aca81543b880a579f634fc
	Others: Installed by apt
