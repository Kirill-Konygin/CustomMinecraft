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
- `FVoxelWorldData` manages chunks and world-to-local coordinate conversion.
- `AVoxelTerrain` generates terrain and coordinates world updates.
- `UVoxelInstanceRenderer` renders cubes with instanced static meshes.
- `UVoxelInstanceCollision` maintains a pool of collision boxes near the player.
- `APlayerPawn` handles movement, mining, and cube placement.

Cube properties and generation layers are configured through a Data Table and Gameplay Tags.

## Known Limitations

- Layer ranges must have valid matching cube definitions. Incomplete configuration is not validated at runtime.
- Hidden-voxel culling does not cross chunk boundaries.
- Generated chunk data remains cached in memory for the current session.

## Future Improvements

- Cross-chunk hidden-voxel culling.
- Save/Load system.
