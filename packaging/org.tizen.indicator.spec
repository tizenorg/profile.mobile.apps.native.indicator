%define PREFIX    /usr/apps/%{name}
%define RESDIR    %{PREFIX}/res
%define PREFIXRW  /opt/apps/%{name}

Name:       org.tizen.indicator
Summary:    indicator window
Version:    0.1.47
Release:    1
Group:      utils
License:    Flora Software License
Source0:    %{name}-%{version}.tar.gz
Source101:  indicator.service

BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-appfw-app-manager)
BuildRequires:  pkgconfig(capi-system-runtime-info)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(ecore-evas)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(minicontrol-monitor)
BuildRequires:  pkgconfig(icu-io)
BuildRequires:  pkgconfig(feedback)

BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: gettext-tools

Requires(post): /usr/bin/vconftool
%description
indicator window.

%prep
%setup -q

%build
LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DCMAKE_INSTALL_PREFIXRW=%{PREFIXRW}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE.Flora %{buildroot}/usr/share/license/%{name}

mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/
mkdir -p %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/
ln -s ../../init.d/indicator %{buildroot}/%{_sysconfdir}/rc.d/rc5.d/S01indicator
ln -s ../../init.d/indicator %{buildroot}/%{_sysconfdir}/rc.d/rc3.d/S44indicator

install -d %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants
install -m0644 %{SOURCE101} %{buildroot}%{_libdir}/systemd/user/
ln -sf ../indicator.service %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/indicator.service

%clean
rm -rf %{buildroot}

%post
vconftool set -t int memory/radio/state 0 -i -g 6518
vconftool set -t int memory/music/state 0 -i -g 6518
vconftool set -t int memory/private/%{name}/home_pressed 0 -i -g 6518
vconftool set -t bool memory/private/%{name}/started 0 -i -u 5000

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.indicator.manifest
%defattr(-,root,root,-)
%{PREFIX}/bin/*
%{RESDIR}/locale/*
%{RESDIR}/icons/*
%{RESDIR}/edje/*
/usr/share/packages/%{name}.xml
%attr(775,app,app) %{PREFIXRW}/data
%attr(755,-,-) %{_sysconfdir}/init.d/indicator
%{_sysconfdir}/rc.d/rc5.d/S01indicator
%{_sysconfdir}/rc.d/rc3.d/S44indicator
%{_libdir}/systemd/user/core-efl.target.wants/indicator.service
%{_libdir}/systemd/user/indicator.service
/usr/share/license/%{name}

