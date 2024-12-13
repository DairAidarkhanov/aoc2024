const input = Deno.readTextFileSync("input.txt");

function findAntennas(grid) {
  const antennas = new Map();
  for (let y = 0; y < grid.length; ++y) {
    for (let x = 0; x < grid[y].length; ++x) {
      const char = grid[y][x];
      if (char !== ".") {
        if (!antennas.has(char)) {
          antennas.set(char, []);
        }
        antennas.get(char).push({ x, y });
      }
    }
  }
  return antennas;
}

function arePointsAligned(p1, p2, p3) {
  const dx1 = p2.x - p1.x;
  const dy1 = p2.y - p1.y;
  const dx2 = p3.x - p2.x;
  const dy2 = p3.y - p2.y;

  return dx1 * dy2 === dx2 * dy1;
}

function getDistance(p1, p2) {
  const dx = p2.x - p1.x;
  const dy = p2.y - p1.y;
  return Math.sqrt(dx * dx + dy * dy);
}

function findAntinodesBetweenAntennas(ant1, ant2, gridSize) {
  const antinodes = [];

  for (let y = 0; y < gridSize.height; ++y) {
    for (let x = 0; x < gridSize.width; ++x) {
      const point = { x, y };

      if (!arePointsAligned(ant1, point, ant2)) continue;

      const d1 = getDistance(ant1, point);
      const d2 = getDistance(ant2, point);

      const ratio = Math.max(d1, d2) / Math.min(d1, d2);
      if (Math.abs(ratio - 2) < 0.0001) {
        antinodes.push(point);
      }
    }
  }

  return antinodes;
}

function findAntinodes2(ant1, ant2, gridSize) {
  const antinodes = [];

  antinodes.push({ ...ant1 });
  antinodes.push({ ...ant2 });

  for (let y = 0; y < gridSize.height; ++y) {
    for (let x = 0; x < gridSize.width; ++x) {
      const point = { x, y };
      if (arePointsAligned(ant1, point, ant2)) {
        antinodes.push(point);
      }
    }
  }

  return antinodes;
}

function findAllAntinodes(grid, isPart2 = false) {
  const antennas = findAntennas(grid);
  const antinodeSet = new Set();
  const gridSize = {
    height: grid.length,
    width: grid[0].length,
  };

  for (const [_freq, antennaPositions] of antennas.entries()) {
    if (antennaPositions.length < 2) continue;

    for (let i = 0; i < antennaPositions.length - 1; ++i) {
      for (let j = i + 1; j < antennaPositions.length; ++j) {
        const antinodes = isPart2
          ? findAntinodes2(antennaPositions[i], antennaPositions[j], gridSize)
          : findAntinodesBetweenAntennas(
            antennaPositions[i],
            antennaPositions[j],
            gridSize,
          );

        for (const node of antinodes) {
          if (
            node.x >= 0 && node.x < gridSize.width &&
            node.y >= 0 && node.y < gridSize.height
          ) {
            antinodeSet.add(`${node.x},${node.y}`);
          }
        }
      }
    }
  }

  return antinodeSet;
}

const grid = input.trim().split("\n").map((line) => line.trim().split(""));

const antinodesPartOne = findAllAntinodes(grid, false);
console.log(
  `Number of unique antinode locations (A-frequency) =\n\t${antinodesPartOne.size}`,
);

const antinodesPartTwo = findAllAntinodes(grid, true);
console.log(
  `Number of unique antinode locations (T-frequency) =\n\t${antinodesPartTwo.size}`,
);
