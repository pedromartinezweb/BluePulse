# BluePulse

BluePulse es una aplicacion para Windows que ayuda a mantener la sesion activa y evitar que la pantalla entre en reposo por inactividad.

## Para que sirve

BluePulse esta pensada para mantener el equipo activo durante periodos de inactividad sin necesidad de permisos de administrador.

## Compilacion

Requisitos:

- Windows x64.
- `gcc` y `windres` disponibles en `PATH`, o un ZIP de WinLibs/MinGW dentro de `tools/`.

Compilar desde PowerShell:

```powershell
.\build.ps1
```

El ejecutable generado queda en `dist\BluePulse.exe`.

## Firma de Windows

El ejecutable incluye metadatos de producto en `assets\windows\BluePulse.rc`.

Para firmar el binario en GitHub Actions necesitas un certificado Authenticode en formato `.pfx` y estos secretos del repositorio:

- `WINDOWS_SIGNING_CERTIFICATE_BASE64`: contenido del `.pfx` codificado en Base64.
- `WINDOWS_SIGNING_CERTIFICATE_PASSWORD`: contrasena del certificado.

En Windows puedes generar el valor Base64 asi:

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes('certificado.pfx')) | Set-Clipboard
```

El workflow firma `dist\BluePulse.exe` antes de publicar la release cuando esos secretos existen.

## Limpieza

Para borrar artefactos generados por la compilacion:

```powershell
.\build.ps1 -Clean
```

Para una limpieza mas completa:

```powershell
.\build.ps1 -Clean -Deep
```

## Licencia

BluePulse se distribuye bajo una licencia gratuita de uso empresarial interno.
Consulta `LICENSE` para el texto completo.
