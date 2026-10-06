# Crea la estructura de carpetas del capstone (Nivel 1)
# Uso: abre PowerShell en la carpeta raiz del proyecto (donde esta el .uproject) y corre:
#   powershell -ExecutionPolicy Bypass -File .\Create-Folders.ps1

$uproject = Get-ChildItem -Filter *.uproject | Select-Object -First 1
if (-not $uproject) {
    Write-Host "No encontre un .uproject aqui. Abre PowerShell en la carpeta raiz del proyecto." -ForegroundColor Red
    exit 1
}
$moduleName = $uproject.BaseName
Write-Host "Proyecto: $moduleName"

# Carpetas de Content (todo lo nuestro va bajo Content/Game para no mezclarlo con el template)
$contentFolders = @(
    "Content/Game/Characters/Player",
    "Content/Game/Characters/Enemies",
    "Content/Game/Combat/Animations",
    "Content/Game/Combat/Montages",
    "Content/Game/Abilities/Spells",
    "Content/Game/AI/BehaviorTrees",
    "Content/Game/AI/Blackboards",
    "Content/Game/Dungeon/Rooms",
    "Content/Game/Dungeon/Props",
    "Content/Game/Dungeon/Generator",
    "Content/Game/Items",
    "Content/Game/UI/HUD",
    "Content/Game/UI/Menus",
    "Content/Game/Data/DataAssets",
    "Content/Game/Data/DataTables",
    "Content/Game/VFX/Niagara",
    "Content/Game/Audio",
    "Content/Game/Maps",
    "Content/Game/Art/Meshes",
    "Content/Game/Art/Materials",
    "Content/Game/Art/Textures"
)

# Carpetas de codigo C++ dentro del modulo principal
$sourceFolders = @(
    "Source/$moduleName/Core",
    "Source/$moduleName/Characters",
    "Source/$moduleName/Combat",
    "Source/$moduleName/Abilities",
    "Source/$moduleName/AI",
    "Source/$moduleName/Dungeon",
    "Source/$moduleName/Inventory",
    "Source/$moduleName/UI",
    "Source/$moduleName/Data"
)

# Carpetas de documentacion fuera del motor
$docFolders = @(
    "Docs/Research",
    "Docs/Design",
    "Docs/Weekly-Notes"
)

foreach ($folder in ($contentFolders + $sourceFolders + $docFolders)) {
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    $keep = Join-Path $folder ".gitkeep"
    if (-not (Test-Path $keep)) { New-Item -ItemType File -Path $keep | Out-Null }
    Write-Host "OK  $folder"
}

Write-Host ""
Write-Host "Listo. Siguiente paso: git add . y git commit." -ForegroundColor Green
