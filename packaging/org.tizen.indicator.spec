Name:       org.tizen.indicator
Summary:    indicator window
Version:    0.2.53
Release:    1
Group:      utils
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

%if "%{?profile}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

%if "%{?profile}"=="tv"
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
BuildRequires: pkgconfig(ecore-evas)
BuildRequires: pkgconfig(edje)
BuildRequires: pkgconfig(evas)
BuildRequires: pkgconfig(eina)
BuildRequires: pkgconfig(vconf)
BuildRequires: pkgconfig(notification)
BuildRequires: pkgconfig(minicontrol-monitor)
BuildRequires: pkgconfig(icu-io)
BuildRequires: pkgconfig(feedback)
BuildRequires: pkgconfig(pkgmgr-info)
BuildRequires: pkgconfig(edbus)
BuildRequires: pkgconfig(efl-assist)
BuildRequires: pkgconfig(capi-message-port)
BuildRequires: pkgconfig(tzsh-indicator-service)
BuildRequires: pkgconfig(libtzplatform-config)
BuildRequires: pkgconfig(capi-system-device)
BuildRequires: pkgconfig(capi-telephony)
BuildRequires: pkgconfig(capi-network-wifi-direct)
BuildRequires: pkgconfig(capi-network-nfc)
BuildRequires: pkgconfig(capi-network-tethering)
BuildRequires: pkgconfig(storage)
BuildRequires: pkgconfig(capi-base-utils-i18n)
BuildRequires: pkgconfig(callmgr_client)

BuildRequires: cmake
BuildRequires: edje-tools
BuildRequires: gettext-tools
BuildRequires: hash-signer

Requires(post): /usr/bin/vconftool

%description
Indicator window reference implementation.

%prep
%setup -q

%build

%define _pkg_dir %{TZ_SYS_RO_APP}/%{name}
%define _pkg_shared_dir %{_pkg_dir}/shared
%define _pkg_data_dir %{_pkg_dir}/data
%define _sys_icons_dir %{_pkg_shared_dir}/res
%define _sys_packages_dir %{TZ_SYS_RO_PACKAGES}
%define _sys_license_dir %{TZ_SYS_SHARE}/license
%define tizen_sign 1
%define tizen_sign_base %{_pkg_dir}
%define tizen_sign_level public
%define tizen_author_sign 1
%define tizen_dist_sign 1


cd CMake
cmake . -DINSTALL_PREFIX=%{_pkg_dir} \
	-DSYS_ICONS_DIR=%{_sys_icons_dir} \
	-DSYS_PACKAGES_DIR=%{_sys_packages_dir}
make %{?jobs:-j%jobs}
cd -


%install
rm -rf %{buildroot}
cd CMake
%make_install
cd -

mkdir -p %{buildroot}/%{_sys_license_dir}
cp LICENSE %{buildroot}/%{_sys_license_dir}/%{name}

%find_lang indicator-win


%files -f indicator-win.lang
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_pkg_dir}/bin/*
%{_pkg_dir}/res/resource/icons/*
%{_pkg_dir}/res/resource/*.edj
%{_sys_packages_dir}/%{name}.xml
%{_sys_license_dir}/%{name}
%{_pkg_dir}/author-signature.xml
%{_pkg_dir}/signature1.xml
