package sygs

import chisel3._
import chisel3.util._
import chiseltest._
import chiseltest.simulator.WriteVcdAnnotation
import org.scalatest.flatspec.AnyFlatSpec
import spatial_templates._
import pe._
import Util._
import scala.collection.mutable.ListBuffer
import scala.util.Random

class SyGSControllerTestModule(val numberOfPEs: Int,
                               val accumulatorPerPE: Int,
                               inverterPerPe: Int,
                               val variablesMemBanks: Int,
                               val accumulatorMemAddressWidth: Int,
                               val variablesMemAddressWidth: Int,
                               exp: Int,
                               sig: Int) extends Module {

  val io = IO(new RoCCControllerIO(numberOfPEs, exp + sig))

  private val controller = Module(new SyGSController(numberOfPEs, accumulatorPerPE,
      Math.max(accumulatorMemAddressWidth, log2Up(variablesMemBanks) + variablesMemAddressWidth), exp + sig))
  io <> controller.io

  private val pes = Seq.fill(numberOfPEs)(Module(
    new SyGSPE(accumulatorPerPE, inverterPerPe, variablesMemBanks, accumulatorMemAddressWidth, variablesMemAddressWidth,
      10, 10, false, 10, 2, 8, exp, sig, 3)))
  controller.connectPEs(pes)

  private val brams = Seq.fill(variablesMemBanks)(Module(
    new BRAMLikeMem1(new ElemId(1, 0, 0, 0), exp + sig, variablesMemAddressWidth)))

  private val variablesMemInterface = Module(
    new NtoMMemInterface(exp + sig, variablesMemAddressWidth, variablesMemBanks,
      2 * accumulatorPerPE * numberOfPEs + 1, 5, 0))

  variablesMemInterface.connectMems(brams)
  controller.connectMemInterface(variablesMemInterface, 0)

  private val accumulatorReqs = 9
  for(i <- 0 until numberOfPEs) {

    for(a <- 0 until accumulatorPerPE) {

      val bram = Module(
        new BRAMLikeMem1(new ElemId(1, 0, 0, 0), exp + sig, accumulatorMemAddressWidth))

      val accumulatorMemInterface = Module(
        new NtoMMemInterface(exp + sig, accumulatorMemAddressWidth, 1, accumulatorReqs + 1, 5, 0))

      accumulatorMemInterface.connectMems(Seq(bram))
      controller.connectMemInterface(accumulatorMemInterface, 1 + i * accumulatorPerPE + a)

      pes(i).connectToAccumulatorMemInterface(accumulatorMemInterface, a)
      pes(i).connectToVariablesMemInterface(variablesMemInterface, i, a)
    }
  }
}

class SyGSControllerTest extends AnyFlatSpec with ChiselScalatestTester {

  behavior of "controller"
  it should "write and read memory" in
    test(new SyGSControllerTestModule(2, 1, 1, 2, 7, 7, 11, 53))
      .withAnnotations(Seq(WriteVcdAnnotation)) { c =>

        c.clock.setTimeout(10)

        val data = ListBuffer.fill(4)(Random.nextLong(Long.MaxValue))

        c.io.rocc_cmd.resp_rdy.poke(true.B)

        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)

        for(i <- 0 until c.numberOfPEs * c.accumulatorPerPE + 1) {

          writeMemChunkLong(data, i, roundRobinAddresses = i == 0, c)

          c.io.rocc_cmd.busy.expect(false.B)
          c.io.rocc_cmd.cmd_rdy.expect(true.B)

          checkMemLong(data, i, roundRobinAddresses = i == 0, c)
        }
      }
  it should "produce the right results (forward)" in
    test(new SyGSControllerTestModule(2, 1, 2, 2, 7, 7, 11, 53))
      .withAnnotations(Seq(WriteVcdAnnotation)) { c =>

        test(c, backwards = false, 2, 2, 4)
      }
  it should "produce the right results (backward)" in
    test(new SyGSControllerTestModule(2, 1, 2, 2, 7, 7, 11, 53))
      .withAnnotations(Seq(WriteVcdAnnotation)) { c =>

        test(c, backwards = true, 2, 4, 4)
      }
  it should "work with smol matrix" in
    test(new SyGSControllerTestModule(2, 1, 1, 2, 17, 12, 11, 53))
      .withAnnotations(Seq(WriteVcdAnnotation)) { c =>

        c.clock.setTimeout(300)

        val variables = ListBuffer.fill(4)(0L)
        val acc1Mem = ListBuffer[Long](5L,2L,2L,1L,1L,3L,0L,4715937977367081240L,2L,1L,-4515190131097614053L,4703336661677694213L,4607362562785112228L,4615288898129284301L,4617315517961601024L,4618441417868443648L,0L,1L,3L,0L,1)
        val acc2Mem = ListBuffer[Long](3L,2L,2L,1L,1L,2L,1L,4714871012338188716L,1L,0L,4715303177709975584L,4611686018427387904L,4607182418800017408L,4619567317775286272L,1L,2L,3)

        c.io.rocc_cmd.resp_rdy.poke(true.B)

        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)

        writeMemChunkLong(variables, 0, roundRobinAddresses = true, c)
        //c.clock.step(Random.nextInt(15))
        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)
        checkMemLong(variables, 0, roundRobinAddresses = true, c)
        //c.clock.step(Random.nextInt(15))

        writeMemChunkLong(acc1Mem, 1, roundRobinAddresses = false, c)
        //c.clock.step(Random.nextInt(15))
        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)
        checkMemLong(acc1Mem, 1, roundRobinAddresses = false, c)
        //c.clock.step(Random.nextInt(15))

        writeMemChunkLong(acc2Mem, 2, roundRobinAddresses = false, c)
        //c.clock.step(Random.nextInt(15))
        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)
        checkMemLong(acc2Mem, 2, roundRobinAddresses = false, c)
        //c.clock.step(Random.nextInt(15))

        start(c, backwards = false)
        c.clock.step()

        var cycles = 1
        while (!c.io.rocc_cmd.resp_valid.peekBoolean()) {

          c.clock.step()
          cycles += 1
          println("cycle: " + cycles)
        }

        println("forward done")

        start(c, backwards = true)
        c.clock.step()

        cycles = 1
        while (!c.io.rocc_cmd.resp_valid.peekBoolean()) {

          c.clock.step()
          cycles += 1
          println("cycle: " + cycles)
        }

        println("backwards done");
      }

  it should "work with test matrix" in
    test(new SyGSControllerTestModule(2, 1, 1, 2, 17, 12, 11, 53))
      .withAnnotations(Seq(WriteVcdAnnotation)) { c =>

        c.clock.setTimeout(300)

        val variables = ListBuffer.fill(8)(0L)
        val acc1Mem = ListBuffer[Long](11L,8L,2L,5L,3L,2L,0L,4622382067542392832L,2L,0L,4596734067664517857L,1L,0L,4608128174721765212L,1L,0L,4616439567834077463L,1L,0L,4616427182935102194L,2L,0L,4614370163805300720L,1L,0L,4621256167635550208L,1L,0L,4620852431187955511L,4703336661677694213L,3126904842697212900L,4608168707118411547L,4605544729831520401L,4593383389541754208L,-4629559943611654303L,4595758375415326996L,-4564734054752660016L,4621419423122042388L,4595760638744355727L,-4624638370955705450L,0L,1L,1L,2L,2L,3L,4L,5L,6L,6L,7L)
        val acc2Mem = ListBuffer[Long](4L,5L,2L,5L,0L,2L,0L,4614370163805300720L,1L,0L,4621256167635550208L,1L,0L,4620852431187955511L,0L,0L,0L,0L,0L,81L,-4564734054752660016L,4621419423122042388L,4595760638744355727L,-4624638370955705450L,5L,6L,6L,7L)

        c.io.rocc_cmd.resp_rdy.poke(true.B)

        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)

        writeMemChunkLong(variables, 0, roundRobinAddresses = true, c)
        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)

        writeMemChunkLong(acc1Mem, 1, roundRobinAddresses = false, c)
        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)

        writeMemChunkLong(acc2Mem, 2, roundRobinAddresses = false, c)
        c.io.rocc_cmd.busy.expect(false.B)
        c.io.rocc_cmd.cmd_rdy.expect(true.B)

        start(c, backwards = false)
        c.clock.step()

        var cycles = 1
        while (!c.io.rocc_cmd.resp_valid.peekBoolean()) {

          c.clock.step()
          cycles += 1
          println("cycle: " + cycles)
        }

        println("forward done")

        start(c, backwards = true)
        c.clock.step()

        cycles = 1
        while (!c.io.rocc_cmd.resp_valid.peekBoolean()) {

          c.clock.step()
          cycles += 1
          println("cycle: " + cycles)
        }

        println("backwards done");
      }

  def test(c: SyGSControllerTestModule, backwards: Boolean, colors: Int, rowsPerColor: Int, numberOfNonZerosPerRow: Int): Unit = {

    c.clock.setTimeout(1000)

    val numberOfAccumulators = c.numberOfPEs * c.accumulatorPerPE

    val rowsPerColorPerAccumulator = rowsPerColor / numberOfAccumulators
    val matrixDim = rowsPerColor * numberOfNonZerosPerRow

    val totalNonZerosPerAccumulator = numberOfNonZerosPerRow * rowsPerColorPerAccumulator * colors
    val totalRowsPerAccumulator = rowsPerColorPerAccumulator * colors

    val initSums = ListBuffer.fill(colors)(ListBuffer.fill(rowsPerColor)(Random.nextDouble() * Random.nextInt()))
    val numberOfNonZerosArray = ListBuffer.fill(colors)(ListBuffer.fill(rowsPerColor)(numberOfNonZerosPerRow.longValue()))
    val diagonalIndices = ListBuffer.fill(colors)(ListBuffer[Long]())

    for (color <- 0 until colors)
      for (r <- 0 until rowsPerColor)
        diagonalIndices(color) += Random.nextLong(numberOfNonZerosArray(color)(r))

    val diagonalValues = ListBuffer.fill(colors)(ListBuffer.fill(rowsPerColor)(0d))
    val diagonalColumnIndices = ListBuffer.fill(colors)(ListBuffer.fill(rowsPerColor)(0L))
    val rowValuesArray = ListBuffer.fill(colors)(ListBuffer.fill(rowsPerColor)(ListBuffer[java.lang.Double]()))
    val columnIndicesArray = ListBuffer.fill(colors)(ListBuffer.fill(rowsPerColor)(ListBuffer[Long]()))
    val variables = ListBuffer.fill(matrixDim)(Random.nextDouble() * Random.nextInt())
    val resultVariables = variables.clone()

    c.io.rocc_cmd.resp_rdy.poke(true.B)

    c.io.rocc_cmd.busy.expect(false.B)
    c.io.rocc_cmd.cmd_rdy.expect(true.B)

    writeMemChunkDouble(variables, 0, roundRobinAddresses = true, c)
    println("variables (double): " + variables)
    println("variables (int)  : " + variables.map(v => doubleToUInt(v)))
    println()

    for (a <- 0 until numberOfAccumulators) {

      writeMem(totalNonZerosPerAccumulator, 0, a + 1, c)
      c.clock.step()
    }

    for (a <- 0 until numberOfAccumulators) {

      writeMem(totalRowsPerAccumulator, 1, a + 1, c)
      c.clock.step()
    }

    for (a <- 0 until numberOfAccumulators) {

      writeMem(colors, 2, a + 1, c)
      c.clock.step()
    }

    for (color <- 0 until colors) {

      for (a <- 0 until numberOfAccumulators) {

        writeMem(rowsPerColorPerAccumulator, 3 + color, a + 1, c)
        c.clock.step()
      }
    }

    for (color <- 0 until colors) {

      for (r <- 0 until rowsPerColorPerAccumulator) {

        val rowIndices = ListBuffer.fill(numberOfAccumulators)(0)
        val offset = 3 + colors + 3 * (color * rowsPerColorPerAccumulator + r)

        for (a <- 0 until numberOfAccumulators) {

          rowIndices(a) = r + a * rowsPerColorPerAccumulator

          writeMem(numberOfNonZerosArray(color)(rowIndices(a)), offset, a + 1, c)
          c.clock.step()
        }

        for (a <- 0 until numberOfAccumulators) {

          rowIndices(a) = r + a * rowsPerColorPerAccumulator

          writeMem(diagonalIndices(color)(rowIndices(a)), offset + 1, a + 1, c)
          c.clock.step()
        }

        for (a <- 0 until numberOfAccumulators) {

          writeMem(initSums(color)(rowIndices(a)), offset + 2, a + 1, c)
          c.clock.step()
        }
      }
    }

    for (color <- 0 until colors) {

      for (r <- 0 until rowsPerColorPerAccumulator) {

        val rowIndices = ListBuffer.fill(numberOfAccumulators)(0)
        val offset = 3 + colors + 3 * totalRowsPerAccumulator +
          (rowsPerColorPerAccumulator * color + r) * numberOfNonZerosPerRow

        for (i <- 0 until numberOfNonZerosPerRow) {

          for (a <- 0 until numberOfAccumulators) {

            rowIndices(a) = r + a * rowsPerColorPerAccumulator

            val numberOfNonZeros = numberOfNonZerosArray(color)(rowIndices(a))
            val diagonalIndex = diagonalIndices(color)(rowIndices(a))
            val rowValues = rowValuesArray(color)(rowIndices(a))

            val rowValue = (Random.nextDouble() + 0.1f) * Random.nextInt()

            if (i == diagonalIndex) diagonalValues(color)(rowIndices(a)) = rowValue
            else if (i < numberOfNonZeros) rowValues += rowValue

            if (i < numberOfNonZeros) {

              writeMem(rowValue, offset + i, a + 1, c)
              c.clock.step()
            }
          }
        }
      }
    }

    for (color <- 0 until colors) {

      for (r <- 0 until rowsPerColorPerAccumulator) {

        val rowIndices = ListBuffer.fill(numberOfAccumulators)(0)
        val offset = 3 + colors + 3 * totalRowsPerAccumulator +
          (rowsPerColorPerAccumulator * color + r) * numberOfNonZerosPerRow + totalNonZerosPerAccumulator

        for (a <- 0 until numberOfAccumulators) {

          rowIndices(a) = r + a * rowsPerColorPerAccumulator

          val numberOfNonZeros = numberOfNonZerosArray(color)(rowIndices(a))
          val columnIndices = columnIndicesArray(color)(rowIndices(a))

          for (i <- 0 until numberOfNonZeros.toInt)
            columnIndices += (i + (r + a * rowsPerColorPerAccumulator) * numberOfNonZerosPerRow)
        }

        for (i <- 0 until numberOfNonZerosPerRow) {

          for (a <- 0 until numberOfAccumulators) {

            val numberOfNonZeros = numberOfNonZerosArray(color)(rowIndices(a))
            val columnIndices = columnIndicesArray(color)(rowIndices(a))

            val columnIndex = if (i < numberOfNonZeros) columnIndices(i) else 0

            if (i < numberOfNonZeros) {

              writeMem(columnIndex, offset + i, a + 1, c)
              c.clock.step()
            }
          }
        }

        c.io.rocc_cmd.cmd_valid.poke(false.B)

        for (a <- 0 until numberOfAccumulators) {

          val numberOfNonZeros = numberOfNonZerosArray(color)(rowIndices(a))
          val rowValues = rowValuesArray(color)(rowIndices(a))
          val columnIndices = columnIndicesArray(color)(rowIndices(a))
          val diagonalIndex = diagonalIndices(color)(rowIndices(a))
          val initSum = initSums(color)(rowIndices(a))

          diagonalColumnIndices(color)(rowIndices(a)) = columnIndices.remove(diagonalIndex.toInt)
          columnIndicesArray(color)(rowIndices(a)) = columnIndices

          println("numberOfNonZeros: " + numberOfNonZeros)
          println("diagonalValueIndex: " + diagonalIndex)
          println("diagonalValue: " + diagonalValues(color)(rowIndices(a)) + " " + doubleToUInt(diagonalValues(color)(rowIndices(a))))
          println("diagonalValueColumnIndex: " + diagonalColumnIndices(color)(rowIndices(a)))
          println("initSum: " + initSum + " " + doubleToUInt(initSums(color)(rowIndices(a))))
          println("rowValues (double): " + rowValues)
          println("rowValues (int)  : " + rowValues.map(doubleToUInt(_)))
          println("columnIndices: " + columnIndices)
          println()
        }
      }
    }

    c.io.rocc_cmd.cmd_valid.poke(false.B)

    //calculate results
    if (backwards) {

      for (color <- colors - 1 to 0 by -1) {

        for (r <- rowsPerColorPerAccumulator - 1 to 0 by -1) {

          val rowIndices = ListBuffer.fill(numberOfAccumulators)(0)

          for (a <- numberOfAccumulators - 1 to 0 by -1) {

            rowIndices(a) = r + a * rowsPerColorPerAccumulator

            val numberOfNonZeros = numberOfNonZerosArray(color)(rowIndices(a))
            val rowValues = rowValuesArray(color)(rowIndices(a))
            val columnIndices = columnIndicesArray(color)(rowIndices(a))
            var initSum = initSums(color)(rowIndices(a))
            var sum2 = 0d

            for (i <- numberOfNonZeros - 2 to 0 by -1) {

              val temp = sum2
              sum2 = initSum - (rowValues(i.toInt) * resultVariables(columnIndices(i.toInt).toInt))
              initSum = temp
            }

            resultVariables(diagonalColumnIndices(color)(rowIndices(a)).toInt) = 1 / diagonalValues(color)(rowIndices(a)) * (initSum + sum2)
          }
        }
      }
    } else {

      for (color <- 0 until colors) {

        for (r <- 0 until rowsPerColorPerAccumulator) {

          val rowIndices = ListBuffer.fill(numberOfAccumulators)(0)

          for (a <- 0 until numberOfAccumulators) {

            rowIndices(a) = r + a * rowsPerColorPerAccumulator

            val numberOfNonZeros = numberOfNonZerosArray(color)(rowIndices(a))
            val rowValues = rowValuesArray(color)(rowIndices(a))
            val columnIndices = columnIndicesArray(color)(rowIndices(a))
            var initSum = initSums(color)(rowIndices(a))
            var sum2 = 0d

            for (i <- 0 until numberOfNonZeros.toInt - 1) {

              val temp = sum2
              sum2 = initSum - (rowValues(i.toInt) * resultVariables(columnIndices(i.toInt).toInt))
              initSum = temp
            }

            resultVariables(diagonalColumnIndices(color)(rowIndices(a)).toInt) = 1 / diagonalValues(color)(rowIndices(a)) * (initSum + sum2)
          }
        }
      }
    }

    c.clock.step()
    c.io.rocc_cmd.cmd_rdy.expect(true)

    start(c, backwards)
    c.clock.step()

    var cycles = 1
    while (c.io.rocc_cmd.busy.peekBoolean()) {

      c.clock.step()
      cycles += 1
      println("cycle: " + cycles)
    }

    while (!c.io.rocc_cmd.resp_valid.peekBoolean()) {
      c.clock.step()
      cycles += 1
      println("cycle: " + cycles)
    }

    c.io.rocc_cmd.resp_valid.expect(true.B)
    println("Computation ended!\nDuration: " + c.io.rocc_cmd.resp_data.peek().litValue + " cycles")

    checkMemDouble(resultVariables, 0, roundRobinAddresses = true, c)
    println("variables (double): " + resultVariables)
    println("variables (int)  : " + resultVariables.map(v => doubleToUInt(v)))
    println()
  }

  def start(c: SyGSControllerTestModule, backwards: Boolean): Unit = {

    c.io.rocc_cmd.cmd_inst.opcode.poke(SyGSOpCode.EXEC.id.U)
    c.io.rocc_cmd.cmd_inst.funct.poke(if(backwards) 1.U else 0.U)
    c.io.rocc_cmd.cmd_rs1.poke(0.U)
    c.io.rocc_cmd.cmd_rs2.poke(0.U)
    c.io.rocc_cmd.cmd_valid.poke(true.B)

    c.clock.step()

    c.io.rocc_cmd.cmd_valid.poke(false.B)

    println("Start cmd received: " + (if(backwards) "backwards" else "forward"))
  }

  def writeMem(value: Double, index: Int, interface: Int, c: SyGSControllerTestModule): Unit = {

    writeMem(doubleToUInt(value), index, interface, c)
  }

  def writeMem(value: Long, index: Int, interface: Int, c: SyGSControllerTestModule): Unit = {

    writeMem(longToUInt(value), index, interface, c)
  }

  def writeMem(value: UInt, index: Int, interface: Int, c: SyGSControllerTestModule): Unit = {

    c.io.rocc_cmd.cmd_inst.opcode.poke(SyGSOpCode.WREQ.id.U)
    c.io.rocc_cmd.cmd_inst.funct.poke(interface)
    c.io.rocc_cmd.cmd_rs1.poke(index)
    c.io.rocc_cmd.cmd_rs2.poke(0.U)
    c.io.rocc_cmd.cmd_valid.poke(true.B)

    c.clock.step()
    c.io.rocc_cmd.cmd_valid.poke(false.B)

    while(!c.io.rocc_cmd.resp_valid.peekBoolean())
      c.clock.step()

    val dataIn1 = value.litValue >> 32
    println(dataIn1)
    val dataIn2 = value
    println(dataIn2.litValue)

    c.io.rocc_cmd.cmd_inst.opcode.poke(SyGSOpCode.WMEM.id.U)
    c.io.rocc_cmd.cmd_inst.funct.poke(0.U)
    c.io.rocc_cmd.cmd_rs1.poke(dataIn1)
    c.io.rocc_cmd.cmd_rs2.poke(dataIn2)
    c.io.rocc_cmd.cmd_valid.poke(true.B)

    c.clock.step()
    c.io.rocc_cmd.cmd_valid.poke(false.B)

    while(!c.io.rocc_cmd.resp_valid.peekBoolean())
      c.clock.step()
  }

  def writeMemChunkFloat(data: ListBuffer[java.lang.Float], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    writeMemChunkUInt(data.map(floatToUInt), interface, roundRobinAddresses, c)
  }

  def writeMemChunkDouble(data: ListBuffer[Double], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    writeMemChunkUInt(data.map(doubleToUInt), interface, roundRobinAddresses, c)
  }

  def writeMemChunkInt(data: ListBuffer[Int], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    writeMemChunkUInt(data.map(intToUInt), interface, roundRobinAddresses, c)
  }

  def writeMemChunkLong(data: ListBuffer[Long], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    writeMemChunkUInt(data.map(longToUInt), interface, roundRobinAddresses, c)
  }

  def writeMemChunkUInt(data: ListBuffer[UInt], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    var indexInBank = 0
    var bankIndex = 0
    var varIndex = 0

    println("Writing memory interface " + interface + "\n")
    for (i <- data.indices) {

      if (i < data.size) {

        if(roundRobinAddresses) {

          indexInBank = i / c.variablesMemBanks
          bankIndex = i % c.variablesMemBanks
          varIndex = indexInBank + Math.pow(2, c.variablesMemAddressWidth).toInt * bankIndex
        }
        else varIndex = i

        writeMem(data(i), varIndex, interface, c)
      }
    }
  }

  def checkMemFloat(data: ListBuffer[java.lang.Float], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    checkMemUInt(data.map(floatToUInt), interface, roundRobinAddresses, c)
  }

  def checkMemDouble(data: ListBuffer[Double], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    checkMemUInt(data.map(doubleToUInt), interface, roundRobinAddresses, c)
  }

  def checkMemInt(data: ListBuffer[Int], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    checkMemUInt(data.map(intToUInt), interface, roundRobinAddresses, c)
  }

  def checkMemLong(data: ListBuffer[Long], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    checkMemUInt(data.map(longToUInt), interface, roundRobinAddresses, c)
  }

  def checkMemUInt(correctData: ListBuffer[UInt], interface: Int, roundRobinAddresses: Boolean, c: SyGSControllerTestModule): Unit = {

    var indexInBank = 0
    var bankIndex = 0
    var varIndex = 0

    println("\nChecking memory interface " + interface)
    for (i <- correctData.indices) {

      if(i < correctData.size) {

        if(roundRobinAddresses) {

          indexInBank = i / c.variablesMemBanks
          bankIndex = i % c.variablesMemBanks
          varIndex = indexInBank + Math.pow(2, c.variablesMemAddressWidth).toInt * bankIndex
        }
        else varIndex = i

        c.io.rocc_cmd.cmd_inst.opcode.poke(SyGSOpCode.RMEM.id.U)
        c.io.rocc_cmd.cmd_inst.funct.poke(interface.U)
        c.io.rocc_cmd.cmd_rs1.poke(varIndex.U)
        c.io.rocc_cmd.cmd_rs2.poke(0.U)
        c.io.rocc_cmd.cmd_valid.poke(true.B)

        c.clock.step()
        c.io.rocc_cmd.cmd_valid.poke(false.B)

        while(!c.io.rocc_cmd.resp_valid.peekBoolean())
          c.clock.step()

        var res = c.io.rocc_cmd.resp_data.peekInt() << 32
        //println(c.io.rocc_cmd.resp_data.peekInt())
        println(res)

        c.io.rocc_cmd.cmd_inst.opcode.poke(SyGSOpCode.RMEM.id.U)
        c.io.rocc_cmd.cmd_inst.funct.poke(interface.U)
        c.io.rocc_cmd.cmd_rs1.poke(varIndex.U)
        c.io.rocc_cmd.cmd_rs2.poke(1.U)
        c.io.rocc_cmd.cmd_valid.poke(true.B)

        c.clock.step()
        c.io.rocc_cmd.cmd_valid.poke(false.B)

        while(!c.io.rocc_cmd.resp_valid.peekBoolean())
          c.clock.step()

        res += c.io.rocc_cmd.resp_data.peekInt()

        //println(c.io.rocc_cmd.resp_data.peekInt())
        println(res)
        assert(res == correctData(i).litValue)
      }

      c.clock.step()

      c.io.rocc_cmd.cmd_valid.poke(false.B)
    }
  }
}
