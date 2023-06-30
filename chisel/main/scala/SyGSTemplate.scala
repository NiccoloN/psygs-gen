package sygs

import chisel3._
import chisel3.util._
import spatial_templates._
import pe._

/**
  * This is the complete Template of our architecture.
  * It contains a Controller connected with one or more Processing Elements and
  * all the BRAMs instantiated (Local and Shared) connecting all the components together
  */
class SyGSTemplate(numberOfPEs: Int,
                   accumulatorPerPE: Int,
                   inverterPerPE: Int,
                   multiplierQueueSizeDFE: Int,
                   accumulatorQueueSizeDFE: Int,
                   useBlackBoxAdder64: Boolean,
                   dividerQueueSizeDFE: Int,
                   metadataQueueSizeAG: Int,
                   columnIndicesQueueSizeAG: Int,
                   variablesMemBanks: Int,
                   accumulatorMemAddressWidth: Int,
                   variablesMemAddressWidth: Int,
                   floatExpWidth: Int,
                   floatSignWidth: Int,
                   memReqQsize: Int,
                   memRespQsize: Int,
                   use_bb: Boolean) extends Module {

  val ctrl_io = IO(new RoCCControllerIO(numberOfPEs))

  private val controller = Module(new SyGSController(numberOfPEs, accumulatorPerPE,
    Math.max(accumulatorMemAddressWidth, log2Up(variablesMemBanks) + variablesMemAddressWidth), floatExpWidth + floatSignWidth))
  ctrl_io <> controller.io

  private val pes = Seq.fill(numberOfPEs)(Module(
    new SyGSPE(accumulatorPerPE, inverterPerPE, variablesMemBanks, accumulatorMemAddressWidth, variablesMemAddressWidth,
      multiplierQueueSizeDFE, accumulatorQueueSizeDFE, useBlackBoxAdder64, dividerQueueSizeDFE, metadataQueueSizeAG, columnIndicesQueueSizeAG, floatExpWidth, floatSignWidth, memRespQsize)))
  controller.connectPEs(pes)

  private val brams = if(use_bb) {
    Seq.fill(variablesMemBanks)(Module(
    new BRAMLikeMem2(new ElemId(1, 0, 0, 0), floatExpWidth + floatSignWidth, variablesMemAddressWidth)))
  } else {
    Seq.fill(variablesMemBanks)(Module(
    new BRAMLikeMem1(new ElemId(1, 0, 0, 0), floatExpWidth + floatSignWidth, variablesMemAddressWidth)))
  }

  private val variablesMemInterface = Module(
    new NtoMMemInterface(floatExpWidth + floatSignWidth, variablesMemAddressWidth, variablesMemBanks,
      2 * accumulatorPerPE * numberOfPEs + 1, memReqQsize, 0))

  variablesMemInterface.connectMems(brams)
  controller.connectMemInterface(variablesMemInterface, 0)

  private val accumulatorReqs = 9
  for(i <- 0 until numberOfPEs) {

    for(a <- 0 until accumulatorPerPE) {

      val bram = if(use_bb) {
        Module(
        new BRAMLikeMem2(new ElemId(1, 0, 0, 0), floatExpWidth + floatSignWidth, accumulatorMemAddressWidth))
      } else {
        Module(
        new BRAMLikeMem1(new ElemId(1, 0, 0, 0), floatExpWidth + floatSignWidth, accumulatorMemAddressWidth))
      }

      val accumulatorMemInterface = Module(
        new NtoMMemInterface(floatExpWidth + floatSignWidth, accumulatorMemAddressWidth, 1, accumulatorReqs + 1, memReqQsize, 0))

      accumulatorMemInterface.connectMems(Seq(bram))
      controller.connectMemInterface(accumulatorMemInterface, 1 + i * accumulatorPerPE + a)

      pes(i).connectToAccumulatorMemInterface(accumulatorMemInterface, a)
      pes(i).connectToVariablesMemInterface(variablesMemInterface, i, a)
    }
  }
}
