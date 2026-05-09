# Experiments with visualizing metaballs

## Usage

Run `build.sh` and an executable will be installed at
`build/metaball`.

## Cool results

### Smooth blobs

```
reset scene; reset drift; reset camera; set integrator = stratified sampling = 4; add scene = power decay = 16, 2; set drift = camera perpendicular
```

### Camo

```
reset scene; reset drift; reset camera; set integrator = grid = 4; add scene = power decay; set drift = camera perpendicular
```
