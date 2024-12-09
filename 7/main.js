const input = Deno.readTextFileSync("input.txt");

function evaluateExpression(numbers, operators) {
  let result = numbers[0];
  for (let i = 0; i < operators.length; ++i) {
    switch (operators[i]) {
      case "+":
        result += numbers[i + 1];
        break;
      case "*":
        result *= numbers[i + 1];
        break;
      default:
        throw new Error("unreachable");
    }
  }
  return result;
}

function evaluateExpression2(numbers, operators) {
  let result = numbers[0];
  for (let i = 0; i < operators.length; ++i) {
    switch (operators[i]) {
      case "+":
        result += numbers[i + 1];
        break;
      case "*":
        result *= numbers[i + 1];
        break;
      case "||":
        result = parseInt(result.toString() + numbers[i + 1].toString());
        break;
      default:
        throw new Error("unreachable");
    }
  }
  return result;
}

function generateOperatorCombinations(length) {
  const operators = ["+", "*"];
  const combinations = [];

  function generate(current) {
    if (current.length === length) {
      combinations.push([...current]);
      return;
    }
    for (const op of operators) {
      current.push(op);
      generate(current);
      current.pop();
    }
  }

  generate([]);
  return combinations;
}

function generateOperatorCombinations2(length) {
  const operators = ["+", "*", "||"];
  const combinations = [];

  function generate(current) {
    if (current.length === length) {
      combinations.push([...current]);
      return;
    }
    for (const op of operators) {
      current.push(op);
      generate(current);
      current.pop();
    }
  }

  generate([]);
  return combinations;
}

function canSolveEquation(testValue, numbers) {
  const numOperators = numbers.length - 1;
  const operatorCombinations = generateOperatorCombinations(numOperators);

  for (const operators of operatorCombinations) {
    if (evaluateExpression(numbers, operators) === testValue) {
      return true;
    }
  }
  return false;
}

function canSolveEquation2(testValue, numbers) {
  const numOperators = numbers.length - 1;
  const operatorCombinations = generateOperatorCombinations2(numOperators);

  for (const operators of operatorCombinations) {
    if (evaluateExpression2(numbers, operators) === testValue) {
      return true;
    }
  }
  return false;
}

const equations = input.trim().split("\n").map((line) => {
  const [testValue, numbersStr] = line.split(": ");
  return {
    testValue: parseInt(testValue),
    numbers: numbersStr.split(" ").map((n) => parseInt(n)),
  };
});

let totalCalibration = 0;
for (const equation of equations) {
  if (canSolveEquation(equation.testValue, equation.numbers)) {
    totalCalibration += equation.testValue;
  }
}
console.log(`total calibration result (+, *) =\n\t${totalCalibration}`);

totalCalibration = 0;
for (const equation of equations) {
  if (canSolveEquation2(equation.testValue, equation.numbers)) {
    totalCalibration += equation.testValue;
  }
}
console.log(`total calibration result (+, *, ||) =\n\t${totalCalibration}`);
