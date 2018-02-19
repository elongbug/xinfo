Name:       xinfo
Summary:    Window system debug utility.
Version:    0.0.3
Release:    1
Group:      Graphics/X Window System
License:    MIT
Source0:    %{name}-%{version}.tar.gz
BuildRequires: pkgconfig(x11)

# some file to be intalled can be ignored when rpm generates packages
#%define _unpackaged_files_terminate_build 0

%description
Description: Window system debug utility.

%prep
%setup -q

%build

%reconfigure --prefix=%{_prefix} \
    CFLAGS="${CFLAGS} -Wall -Werror"

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install

mkdir -p %{_bindir}

%remove_docs


%files
%manifest xinfo.manifest
%defattr(-,root,root,-)
%{_bindir}/xinfo

