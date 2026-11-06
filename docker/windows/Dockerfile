# escape=`
FROM mcr.microsoft.com/dotnet/framework/runtime:4.8.1-windowsservercore-ltsc2022
SHELL ["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command"]

ENV CARGO_HOME=C:\cargo RUSTUP_HOME=C:\rustup VCPKG_ROOT=C:\Tools\vcpkg
COPY docker/windows/install.ps1 C:/Tools/install.ps1
COPY src/engine/script/rust-toolchain.toml src/vcpkg.json C:/Tools/definitions/
COPY pyproject.toml uv.lock .python-version C:/Tools/python/
RUN C:\Tools\install.ps1

COPY docker/windows/build.ps1 C:/Tools/build.ps1
WORKDIR C:/workspace
ENTRYPOINT ["powershell.exe", "-NoLogo", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", "C:\\Tools\\build.ps1"]
CMD ["-Backend", "directx", "-Config", "Debug", "-Compiler", "clang"]
