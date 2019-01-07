Name:           Intel_Movidius_MyriadX_HDDL-S_Linux
Version:0.6
Release:WW02_2019
Summary:        This is hddl-s projects.
License:        GPL
#URL:
Source0:        %{name}.tar.gz

#BuildRequires:
#Requires:
BuildRoot:  %{_tmppath}/%{name}-root
Prefix:     /usr

%define     usrpath     %{_prefix}
%define     __strip /bin/true

%description

#%debug_package

%prep
%setup -c

%install
install -d $RPM_BUILD_ROOT%{usrpath}
cp -a usr/* $RPM_BUILD_ROOT%{usrpath}

%files
#%defattr(-,root,root)
%{usrpath}/local/bin/*
%{usrpath}/lib/x86_64-linux-gnu/gstreamer-1.0/*
%{usrpath}/lib/x86_64-linux-gnu/libgstcvdl/*
%{usrpath}/include/gstcvdl/*

%config

%post
echo "Setup enviroment variables:"
bash /usr/local/bin/hddls_prepare.sh

%clean
rm -rf $RPM_BUILD_ROOT
rm -rf $RPM_BUILD_DIR/%{name}

%changelog


