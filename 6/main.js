const input = Deno.readTextFileSync("input.txt");

/**
 * @template T, U
 * @typedef {(acc: T, curr: U, index: number) => T} ReducerFn
 */

/** @typedef {0 | 1 | 2 | 3} Direction */
const /** @type {Direction} */ NORTH = 0;
const /** @type {Direction} */ EAST = 1;
const /** @type {Direction} */ SOUTH = 2;
const /** @type {Direction} */ WEST = 3;

/** @typedef {'^' | '>' | 'v' | '<'} Arrow */
const /** @type {readonly Arrow[]} */ ARROWS = ["^", ">", "v", "<"];

/**
 * @param {string} input
 * @returns {[string[][], number]}
 */
function parse_input(input) {
  const rows = input.split("\n").map((row) => row.split(""));
  return [rows, rows.length];
}

/**
 * @param {Arrow} arrow
 */
function arrow_facing(arrow) {
  switch (arrow) {
    case ARROWS[NORTH]:
      return NORTH;
    case ARROWS[EAST]:
      return EAST;
    case ARROWS[SOUTH]:
      return SOUTH;
    case ARROWS[WEST]:
      return WEST;
    default:
      throw new Error("unreachable");
  }
}

/**
 * @param {Arrow} arrow
 */
function turn_arrow_90_degrees(arrow) {
  return ARROWS[(arrow_facing(arrow) + 1) % ARROWS.length];
}

/**
 * @param {string[][]} rows
 */
function find_arrow_coords(rows) {
  /** @type {ReducerFn<[number, number], string[]>} */
  const find_by_arrow = (acc, row, index) => {
    const guardian_found = row.findIndex((char) => ARROWS.includes(char));
    if (guardian_found !== -1) return [index, guardian_found];
    return acc;
  };
  const coords = rows.reduce(find_by_arrow, [-1, -1]);
  return coords;
}

/**
 * @param {string[][]} rows
 * @param {[number, number]} coords
 * @returns {["#" | "." | "X", [number, number]] | null}
 */
function look_around(rows, coords) {
  const [guardian_row, guardian_col] = coords;
  const guardian = rows[guardian_row][guardian_col];
  let next_row, next_col;

  const guardian_facing = arrow_facing(guardian);
  switch (guardian_facing) {
    case NORTH:
      next_row = guardian_row - 1;
      next_col = guardian_col;
      break;
    case EAST:
      next_row = guardian_row;
      next_col = guardian_col + 1;
      break;
    case SOUTH:
      next_row = guardian_row + 1;
      next_col = guardian_col;
      break;
    case WEST:
      next_row = guardian_row;
      next_col = guardian_col - 1;
      break;
    default:
      throw new Error("unreachable");
  }

  if (
    next_row < 0 || next_row >= rows_size ||
    next_col < 0 || next_col >= rows_size
  ) {
    return null;
  }

  const next_symbol = rows[next_row][next_col];
  switch (next_symbol) {
    case "#":
    case ".":
    case "X":
      return [next_symbol, [next_row, next_col]];
    default:
      return null;
  }
}

/**
 * @param {string[][]} test_rows
 * @param {[number, number]} start_coords
 */
function simulate_path(test_rows, start_coords) {
  let [guardian_row, guardian_col] = start_coords;

  const visited = new Set();
  while (true) {
    const current_arrow = test_rows[guardian_row][guardian_col];
    const state = `${guardian_row},${guardian_col},${current_arrow}`;

    if (visited.has(state)) {
      return true;
    }
    if (visited.size > test_rows.length ** 2 * 4) {
      return false;
    }
    visited.add(state);

    const looked_around = look_around(test_rows, [guardian_row, guardian_col]);
    if (!looked_around) return false;

    const [next_symbol, [next_row, next_col]] = looked_around;
    switch (next_symbol) {
      case "#":
        {
          const next_arrow = turn_arrow_90_degrees(current_arrow);
          test_rows[guardian_row][guardian_col] = next_arrow;
        }
        break;
      case ".":
        test_rows[guardian_row][guardian_col] = ".";
        test_rows[next_row][next_col] = current_arrow;
        guardian_row = next_row;
        guardian_col = next_col;
        break;
      default:
        return false;
    }
  }
}

/* part 1 */

let [rows, rows_size] = parse_input(input);
const guardian_coords = find_arrow_coords(rows);

for (
  let [guardian_row, guardian_col] = guardian_coords,
    looked_around = look_around(rows, guardian_coords);;
  [guardian_row, guardian_col] = find_arrow_coords(rows),
    looked_around = look_around(rows, [guardian_row, guardian_col])
) {
  if (!looked_around) {
    rows[guardian_row][guardian_col] = "X";
    break;
  }

  const /** @type {Arrow} */ current_arrow = rows[guardian_row][guardian_col];
  const [next_symbol, [next_row, next_col]] = looked_around;
  switch (next_symbol) {
    case "#":
      rows[guardian_row][guardian_col] = turn_arrow_90_degrees(current_arrow);
      break;
    case ".":
    case "X":
      rows[guardian_row][guardian_col] = "X";
      rows[next_row][next_col] = current_arrow;
      break;
    default:
      throw new Error("unreachable");
  }
}

console.log(
  `number of steps =\n\t${
    rows.flatMap((row) => row.join("")).join("").match(/X/g).length
  }`,
);

/* part 2 */

[rows, rows_size] = parse_input(input);
const [start_guardian_row, start_guardian_col] = find_arrow_coords(rows);

let loop_count = 0;
for (let row = 0; row < rows_size; ++row) {
  for (let col = 0; col < rows_size; ++col) {
    if (
      rows[row][col] !== "." ||
      (row === start_guardian_row && col === start_guardian_col)
    ) {
      continue;
    }

    const test_rows = structuredClone(rows);
    test_rows[row][col] = "#";

    if (simulate_path(test_rows, [start_guardian_row, start_guardian_col])) {
      ++loop_count;
    }
  }
}

console.log(`number of loops =\n\t${loop_count}`);
