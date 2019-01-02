# Don't try fancy stuff like debuginfo, which is useless on binary-only
# packages. Don't strip binary too
# Be sure buildpolicy set to do nothing
%define         _tmppath %{_topdir}/tmp
%define        __spec_install_post %{nil}
%define          debug_package %{nil}
%define        __os_install_post %{_dbpath}/brp-compress

Summary: Why the Fuck is My Network So Slow
Name: whythefuckismynetworksoslow
License: Open-Source
Group: System Environment/Daemons
SOURCE0 : %{name}-%{version}.tar.gz
URL: http://www.sophists.com/



BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
%{summary}

%prep
%setup -q

%build
# Empty section.

%install
rm -rf %{buildroot}
mkdir -p  %{buildroot}

# in builddir
cp -a * %{buildroot}

%clean
rm -rf %{buildroot}

%pre
if [ "$1" = "1" ]; then
   echo "Nothing needed for initial installation" > /dev/null
elif [ "$1" == "2" ]; then
   echo "Stop the service if its already running" > /dev/null
   systemctl stop whythefuckismynetworksoslow
fi

%preun
if [ $1 -eq 0 ] ; then
	systemctl disable whythefuckismynetworksoslow
fi

%post
	systemctl enable whythefuckismynetworksoslow
	systemctl start whythefuckismynetworksoslow

%files
/opt/WhyTheFuckIsMyNetworkSoSlow/WhyTheFuckIsMyNetworkSoSlow-Backend
/usr/lib/systemd/system/whythefuckismynetworksoslow.service
%config(noreplace) /var/opt/WhyTheFuckIsMyNetworkSoSlow/
