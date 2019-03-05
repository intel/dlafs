1. Build RPM package:
     sudo apt-get install elfutils
     hddls_rpm_build.sh <version> <weekly>

2. Install rpm package

    sudo rpm -ivh --nodeps Intel_Movidius_MyriadX_HDDL-S_Linux-0.1-ww41.x86_64.rpm

3. Remove rpm package

    sudo rpm -e Intel_Movidius_MyriadX_HDDL-S_Linux
