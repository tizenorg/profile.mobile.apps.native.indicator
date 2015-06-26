%define PKGNAME org.tizen.indicator
%define PREFIX    /usr/apps/%{PKGNAME}
%define RESDIR    %{PREFIX}/res
%define PREFIXRW  /opt/usr/apps/%{PKGNAME}

Name:       org.tizen.indicator
Summary:    indicator window
Version:    0.2.53
Release:    1
Group:      utils
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1:    indicator.service.system
Source2:    indicator.path
#Source101:  indicator.service

%if "%{?tizen_profile_name}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?tizen_profile_name}"=="tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires: pkgconfig(capi-appfw-application)
BuildRequires: pkgconfig(capi-appfw-app-manager)
BuildRequires: pkgconfig(capi-system-runtime-info)
BuildRequires: pkgconfig(capi-network-bluetooth)
BuildRequires: pkgconfig(capi-appfw-preference)
BuildRequires: pkgconfig(capi-system-system-settings)
BuildRequires: pkgconfig(capi-media-player)
BuildRequires: pkgconfig(capi-media-sound-manager)
BuildRequires: pkgconfig(capi-media-metadata-extractor)
BuildRequires: pkgconfig(capi-network-wifi)
BuildRequires: pkgconfig(capi-ui-efl-util)
BuildRequires: pkgconfig(appcore-common)
BuildRequires: pkgconfig(dlog)
BuildRequires: pkgconfig(elementary)
BuildRequires: pkgconfig(ecore)
#BuildRequires: pkgconfig(ecore-x)
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(libprivilege-control)
BuildRequires: pkgconfig(notification)
#BuildRequires: pkgconfig(utilX)
BuildRequires: pkgconfig(minicontrol-monitor)
BuildRequires: pkgconfig(icu-io)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(efl-assist)
BuildRequires: pkgconfig(tapi)
BuildRequires: pkgconfig(message-port)

BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: gettext-tools
BuildRequires: hash-signer

Requires(post): /usr/bin/vconftool
%description
indicator window.

%prep
%setup -q

%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif

%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

LDFLAGS+="-Wl,--rpath=%{PREFIX}/lib -Wl,--as-needed";export LDFLAGS
CFLAGS+=" -fvisibility=hidden"; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export CXXFLAGS
FFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden"; export FFLAGS

cmake . -DCMAKE_INSTALL_PREFIX=%{PREFIX} -DCMAKE_INSTALL_PREFIXRW=%{PREFIXRW} \


make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE %{buildroot}/usr/share/license/%{PKGNAME}

%define tizen_sign 1
%define tizen_sign_base /usr/apps/%{PKGNAME}
%define tizen_sign_level public
%define tizen_author_sign 1
%define tizen_dist_sign 1

#install -d %{buildroot}/usr/lib/systemd/user/core-efl.target.wants
#install -m0644 %{SOURCE101} %{buildroot}/usr/lib/systemd/user/
#ln -sf ../indicator.service %{buildroot}/usr/lib/systemd/user/core-efl.target.wants/indicator.service
mkdir -p %{buildroot}/usr/lib/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}/usr/lib/systemd/system/indicator.service
ln -s ../indicator.service %{buildroot}/usr/lib/systemd/system/multi-user.target.wants/indicator.service
install -m 0644 %SOURCE2 %{buildroot}/usr/lib/systemd/system/indicator.path
ln -s ../indicator.path %{buildroot}/usr/lib/systemd/system/multi-user.target.wants/

%clean
rm -rf %{buildroot}

%post
vconftool set -t int memory/private/%{PKGNAME}/show_more_noti_port 0 -i -g 6518 -f -s %{PKGNAME}

%postun -p /sbin/ldconfig

%files
%manifest org.tizen.indicator.manifest
%defattr(-,root,root,-)
%{PREFIX}/bin/*
%{RESDIR}/icons/*
%{RESDIR}/edje/*
/usr/share/packages/%{PKGNAME}.xml
%attr(775,app,app) %{PREFIXRW}/data
%attr(755,-,-) %{_sysconfdir}/init.d/indicator
#/usr/lib/systemd/user/core-efl.target.wants/indicator.service
#/usr/lib/systemd/user/indicator.service
/usr/lib/systemd/system/multi-user.target.wants/indicator.service
/usr/lib/systemd/system/indicator.service
/usr/lib/systemd/system/multi-user.target.wants/indicator.path
/usr/lib/systemd/system/indicator.path
/usr/share/license/%{PKGNAME}
/etc/smack/accesses.d/%{PKGNAME}.efl
/usr/apps/%{PKGNAME}/author-signature.xml
/usr/apps/%{PKGNAME}/signature1.xml
/usr/apps/%{PKGNAME}/shared/res/tables/org.tizen.indicator_ChangeableColorInfo.xml
/usr/apps/%{PKGNAME}/shared/res/tables/org.tizen.indicator_ChangeableFontInfo.xml
