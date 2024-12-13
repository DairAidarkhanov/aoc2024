const input = Deno.readTextFileSync("input.txt");

const blocks = new Int32Array(input.length);
let block_id = 0, blocks_bytelength = 0;
for (let i = 0; i < input.length; ++i) {
  const length = parseInt(input[i]);
  if (isNaN(length)) continue;

  const packed = block_id | (length << 28);
  blocks[i] = packed;
  blocks_bytelength += length;
  if ((i & 1) === 0) ++block_id;
}

function format_disk(blocks, blocks_bytelength) {
  const disk = new Int32Array(blocks_bytelength);
  for (let i = 0, offset = 0; i < blocks.length; ++i) {
    const block = blocks[i];
    const [block_id, block_len] = [block & 0xFFFFFFF, (block >> 28) & 0xF];
    disk.fill((i & 1) === 0 ? block_id : -1, offset, offset + block_len);
    offset += block_len;
  }
  return disk;
}

function is_compacted(disk) {
  let foundEmpty = false;
  for (let i = 0; i < disk.length; ++i) {
    if (disk[i] === -1) {
      foundEmpty = true;
    } else {
      if (foundEmpty) {
        return false;
      }
    }
  }
  return true;
}

let disk = format_disk(blocks, blocks_bytelength);

let i = 0, l = 0, r = disk.length - 1;
while (!is_compacted(disk)) {
  while (disk[l] >= 0) {
    ++l;
  }

  while (disk[r] < 0) {
    --r;
  }

  if (disk[l] != null || disk[r] != null) {
    disk[l] = disk[r];
    disk[r] = -1;
  }

  ++i, ++l, --r;
}

function calculate_checksum(disk) {
  let checksum = 0;
  for (let i = 0; i < disk.length; ++i) {
    if (disk[i] >= 0) {
      checksum += disk[i] * i;
    }
  }
  return checksum;
}

console.log(`compaction by blocks =\n\t${calculate_checksum(disk)}`);

function fetch_files(disk) {
  const files = new Map();
  let current_id = null;
  let start = 0, length = 0;
  for (let i = 0; i < disk.length; ++i) {
    if (disk[i] !== current_id) {
      if (current_id !== null && current_id !== -1) {
        files.set(current_id, [start, length]);
      }
      current_id = disk[i];
      start = i, length = 1;
    } else {
      ++length;
    }
  }
  if (current_id !== null && current_id !== -1) {
    files.set(current_id, [start, length]);
  }
  return files;
}

function find_free_space(disk, file_length) {
  let start = 0, current_length = 0;
  for (let i = 0; i < disk.length; ++i) {
    if (disk[i] === -1) {
      if (current_length++ === 0) start = i;
      if (current_length === file_length) return start;
    } else {
      current_length = 0;
    }
  }
  return -1;
}

disk = format_disk(blocks, blocks_bytelength);

const files = fetch_files(disk);
const file_ids = Array.from(files.keys()).sort((a, b) => b - a);
for (const file_id of file_ids) {
  const [file_start, file_length] = files.get(file_id);
  const pos = find_free_space(disk, file_length);

  if (pos !== -1 && pos < file_start) {
    disk.fill(-1, file_start, file_start + file_length);
    disk.fill(file_id, pos, pos + file_length);
  }
}

console.log(`compaction by files =\n\t${calculate_checksum(disk)}`);
