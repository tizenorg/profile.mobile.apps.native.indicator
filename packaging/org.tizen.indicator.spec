%define PREFIX    /usr/apps/%{name}
%define RESDIR    %{PREFIX}/res
%define PREFIXRW  /opt/apps/%{name}

Name:       org.tizen.indicator
Summary:    Indicator Window
Version:    0.1.72
Release:    1
Group:      Application Framework/Utilities
License:    Flora
Source0:    %{name}-%{version}.tar.gz
Source101:  indicator.service
Source102:  org.tizen.indicator.manifest

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
cp %{SOURCE102} .

%build
LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
CFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export CXXFLAGS
FFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export FFLAGS

cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DCMAKE_INSTALL_PREFIXRW=%{PREFIXRW}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

install -d %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants
install -m0644 %{SOURCE101} %{buildroot}%{_libdir}/systemd/user/
ln -sf ../indicator.service %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/indicator.service

%clean
rm -rf %{buildroot}

%post
vconftool set -t int memory/music/state 0 -i -g 6518 -f
vconftool set -t bool memory/private/%{name}/started 0 -i -u 5000 -f
vconftool set -t int memory/private/%{name}/battery_disp 0 -i -u 5000 -f

%files
%manifest org.tizen.indicator.manifest
%defattr(-,root,root,-)
%license LICENSE.Flora NOTICE
%{PREFIX}/bin/*
%{RESDIR}/locale/*
%{RESDIR}/icons/*
%{RESDIR}/edje/*
/usr/share/packages/%{name}.xml
%attr(775,app,app) %{PREFIXRW}/data
%{_libdir}/systemd/user/core-efl.target.wants/indicator.service
%{_libdir}/systemd/user/indicator.service
