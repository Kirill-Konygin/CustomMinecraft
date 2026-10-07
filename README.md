# Custom Minecraft

A small first-person voxel sandbox built in Unreal Engine 5.8.

## Features

- Random terrain generation based on Perlin noise.
- Gray, green, and white cubes selected by height.
- Different mining durations for each cube type.
- Grid-aligned cube mining and placement.
- Fixed world depth and height limits.
- Chunk-based generation and distance-based rendering.
- Instanced rendering.

## Controls

| Action | Input |
| --- | --- |
| Move | `WASD` |
| Move up / down | `Space` / `Left Ctrl` |
| Look | Mouse |
| Mine cube | Hold left mouse button |
| Place cube | Right mouse button |


## Architecture

- `FChunk` stores voxel data using a per-chunk palette.
- `FVoxelWorldData` reads and writes voxel data, keeping chunk storage private. Empty storage remains cached.
- `FVoxelVisibility` keeps the current display area, caches exposed voxels across chunk boundaries, and provides surface queries using existing world data.
- `AVoxelTerrain` calculates display bounds, generates missing areas before rendering, and updates visibility after world changes. Empty terrain does not allocate chunk storage.
- `UVoxelInstanceRenderer` renders cubes with instanced static meshes.
- `UVoxelInstanceCollision` maintains a pool of collision boxes near the player.
- `APlayerPawn` handles movement, mining, and cube placement.

Cube properties and generation layers are configured through a Data Table and Gameplay Tags.

## Known Limitations

- Layer ranges must have valid matching cube definitions. Incomplete configuration is not validated at runtime.
- Generated chunk data remains cached in memory for the current session.

## Future Improvements

- Save/Load system.
