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
