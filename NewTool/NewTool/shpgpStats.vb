Imports System, System.Net, System.Net.Sockets, System.Windows.Forms
Imports System.Text, System.IO, System.Threading
Imports System.Collections, System.Collections.Generic, System.ComponentModel, System.Management
Imports System.Runtime.InteropServices

Public Class shpgpStats
    Public filePath As String = String.Empty

#Region "Structure Declaration"

    <StructLayout(LayoutKind.Explicit)> _
    Public Structure shpgpHalStats

        '   Rx Statistics counters
        <FieldOffset(0)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalRxGoodFrmCnt As UInteger
        <FieldOffset(4)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalRxBytesCnt As UInteger
        <FieldOffset(8)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxGoodDataCnt As UInteger
        <FieldOffset(12)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxGoodBcnCnt As UInteger
        <FieldOffset(16)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxGoodMgmtCnt As UInteger
        <FieldOffset(20)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxGoodSoundCnt As UInteger
        <FieldOffset(24)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxErrBcnCnt As UInteger
        <FieldOffset(28)> <MarshalAsAttribute(UnmanagedType.U4)> Public BcnRxIntCnt As UInteger
        <FieldOffset(32)> <MarshalAsAttribute(UnmanagedType.U4)> Public DuplicateRxCnt As UInteger

        '   Tx Statistics counters
        <FieldOffset(36)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalTxFrmCnt As UInteger
        <FieldOffset(40)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalTxBytesCnt As UInteger
        <FieldOffset(44)> <MarshalAsAttribute(UnmanagedType.U4)> Public TxDataCnt As UInteger
        <FieldOffset(48)> <MarshalAsAttribute(UnmanagedType.U4)> Public TxBcnCnt As UInteger
        <FieldOffset(52)> <MarshalAsAttribute(UnmanagedType.U4)> Public TxMgmtCnt As UInteger
        <FieldOffset(56)> <MarshalAsAttribute(UnmanagedType.U4)> Public TxDataMgmtGoodCnt As UInteger
        <FieldOffset(60)> <MarshalAsAttribute(UnmanagedType.U4)> Public BcnSyncCnt As UInteger
        <FieldOffset(64)> <MarshalAsAttribute(UnmanagedType.U4)> Public BcnSentIntCnt As UInteger

        '   Tx Test Stat
        <FieldOffset(68)> <MarshalAsAttribute(UnmanagedType.U4)> Public CurTxTestFrmCnt As UInteger
        <FieldOffset(72)> <MarshalAsAttribute(UnmanagedType.U1)> Public TxSeqNum As Byte

        '   Rx Test Stat - valid only for single tx-rx setup only 
        <FieldOffset(73)> <MarshalAsAttribute(UnmanagedType.U1)> Public PrevRxSeqNum As Byte
        <FieldOffset(74)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalRxMissCnt As UInteger
        <FieldOffset(78)> <MarshalAsAttribute(UnmanagedType.U4)> Public CorruptFrmCnt As UInteger

        <FieldOffset(82)> <MarshalAsAttribute(UnmanagedType.U4)> Public bpIntCnt As UInteger
        <FieldOffset(83)> <MarshalAsAttribute(UnmanagedType.U1)> Public syncTestValIdx As Byte
        <FieldOffset(84)> <MarshalAsAttribute(UnmanagedType.U4)> Public lastSs1 As UInteger
        <FieldOffset(88)> <MarshalAsAttribute(UnmanagedType.U4)> Public MissSyncCnt As UInteger

        '   rx Phy Active stuck workaround
        <FieldOffset(92)> <MarshalAsAttribute(UnmanagedType.U4)> Public prevBPTotalRxCnt As UInteger

        <FieldOffset(96)> <MarshalAsAttribute(UnmanagedType.U4)> Public STAleadCCOCount As UInteger
        <FieldOffset(100)> <MarshalAsAttribute(UnmanagedType.U4)> Public STAlagCCOCount As UInteger

        <FieldOffset(104)> <MarshalAsAttribute(UnmanagedType.U1)> Public paRxEnHiCnt As Byte
        <FieldOffset(105)> <MarshalAsAttribute(UnmanagedType.U1)> Public phyActHangRstCnt As Byte

        <FieldOffset(106)> <MarshalAsAttribute(UnmanagedType.U2)> Public macTxStuckCnt As UShort
        <FieldOffset(108)> <MarshalAsAttribute(UnmanagedType.U2)> Public macRxStuckCnt As UShort
        <FieldOffset(110)> <MarshalAsAttribute(UnmanagedType.U2)> Public phyStuckCnt As UShort
        <FieldOffset(112)> <MarshalAsAttribute(UnmanagedType.U2)> Public mpiRxStuckCnt As UShort
        <FieldOffset(114)> <MarshalAsAttribute(UnmanagedType.U2)> Public smTxStuckCnt As UShort
        <FieldOffset(116)> <MarshalAsAttribute(UnmanagedType.U2)> Public smRxStuckCnt As UShort
        <FieldOffset(118)> <MarshalAsAttribute(UnmanagedType.U1)> Public macHangRecover1 As Byte
        <FieldOffset(119)> <MarshalAsAttribute(UnmanagedType.U1)> Public macHangRecover2 As Byte

        <FieldOffset(123)> <MarshalAsAttribute(UnmanagedType.U4)> Public PtoHswDropCnt As UInteger
        <FieldOffset(127)> <MarshalAsAttribute(UnmanagedType.U4)> Public HtoPswDropCnt As UInteger
        <FieldOffset(131)> <MarshalAsAttribute(UnmanagedType.U4)> Public GswDropCnt As UInteger

    End Structure

    <StructLayout(LayoutKind.Sequential, pack:=1, size:=32)> _
    Public Structure _plcTxTestResults_t

        <FieldOffset(0)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalRxGoodFrmCnt As UInteger
        <FieldOffset(4)> <MarshalAsAttribute(UnmanagedType.U4)> Public TotalRxBytesCnt As UInteger
        <FieldOffset(8)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxGoodDataCnt As UInteger
        <FieldOffset(12)> <MarshalAsAttribute(UnmanagedType.U4)> Public RxGoodMgmtCnt As UInteger
        <FieldOffset(16)> <MarshalAsAttribute(UnmanagedType.U4)> Public DuplicateRxCnt As UInteger
        <FieldOffset(20)> <MarshalAsAttribute(UnmanagedType.U4)> Public AddrFilterErrCnt As UInteger
        <FieldOffset(24)> <MarshalAsAttribute(UnmanagedType.U4)> Public FrameCtrlErrCnt As UInteger
        <FieldOffset(28)> <MarshalAsAttribute(UnmanagedType.U4)> Public ICVErrCnt As UInteger

    End Structure
#End Region

    '
    '   load
    '
    Sub New()
        Try
            filePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Test Results.txt")
            If Not System.IO.File.Exists(filePath) Then
                System.IO.File.Create(filePath).Dispose()
            End If
        Catch fileException As Exception
            Throw fileException
        End Try
    End Sub

    '
    '   Function to Dump result to file
    '
    Public Sub logResultinTextFile(ByVal m As HomeScreen.metadata)

        If filePath <> m.filePath Then
            filePath = m.filePath
        End If

        Dim sw = New StreamWriter(filePath, True)

        Using sw
            '   Append Metadata
            If m.newTestRun = True Then
                sw.WriteLine("******************* Test Run " & m.runCount & " *******************")
            End If

            sw.WriteLine("Test = " & m.name)
            If m.testStatus = True Then
                sw.WriteLine("Test PASS")
            ElseIf m.testStatus = False Then
                sw.WriteLine("Test FAIL")
            End If
            sw.WriteLine("Test Threshold = " & m.threshold & " %")
            sw.WriteLine("DUT : " & m.DUT & "   " & "REF : " & m.REF)
            sw.WriteLine("Start Time : " & m.startTime & "   " & "Stop Time : " & m.stopTime)
            sw.WriteLine("Test Duration : " & m.duration)
            sw.Write(vbNewLine)

            '   Append TX Params
            sw.WriteLine("Parameters - ")
            sw.Write(vbNewLine)

            If m.testPramas.txpowermode = 2 Then
                sw.WriteLine("High Power Mode")
            ElseIf m.testPramas.txpowermode = 0 Then
                sw.WriteLine("Low Power Mode")
            End If

            If m.testPramas.mcstMode = 1 Then
                sw.WriteLine("Mode = Multicast")
            Else
                sw.WriteLine("Mode = Unicast")
            End If

            sw.WriteLine("Frame Length = " & m.testPramas.frmLen & " Bytes" & "   " & "Inter Frame Delay = " & _
                         m.testPramas.delay & "   " & "Number of Frames = " & m.testPramas.numFrames)

            If m.testPramas.eks.ToString = "15" Then
                sw.WriteLine("Key Index = 0x0F " & "   " & "Encryption = " & _
                         CType([Enum].Parse(GetType(spiTXTestSettings.eSecTestMode), _
                                            m.testPramas.secTestMode), spiTXTestSettings.eSecTestMode).ToString)
            Else
                sw.WriteLine("Key Index = " & m.testPramas.eks.ToString & "   " & "Encryption = " & _
                         CType([Enum].Parse(GetType(spiTXTestSettings.eSecTestMode), _
                                            m.testPramas.secTestMode), spiTXTestSettings.eSecTestMode).ToString)
            End If

            sw.WriteLine("Frame Type = " & _
                         CType([Enum].Parse(GetType(spiTXTestSettings.eFrmType), m.testPramas.frmType),  _
                             spiTXTestSettings.eFrmType).ToString _
                         & "   " & "Length Test Mode = " & _
                         CType([Enum].Parse(GetType(spiTXTestSettings.eLenTestMode), _
                                            m.testPramas.lenTestMode), spiTXTestSettings.eLenTestMode).ToString)
            sw.Write(vbNewLine)

            '   Append Result
            sw.WriteLine("Result of the test - ")
            sw.Write(vbNewLine)
            sw.WriteLine("Total received frames             = " & m.rsp.TotalRxGoodFrmCnt)
            sw.WriteLine("Total received bytes              = " & m.rsp.TotalRxBytesCnt)
            sw.WriteLine("Total received data frames        = " & m.rsp.RxGoodDataCnt)
            sw.WriteLine("Total received management frames  = " & m.rsp.RxGoodMgmtCnt)
            sw.WriteLine("Number of duplicate frames        = " & m.rsp.DuplicateRxCnt)
            sw.WriteLine("Address filter error count        = " & m.rsp.AddrFilterErrCnt)
            sw.WriteLine("Frame control error count         = " & m.rsp.FrameCtrlErrCnt)
            sw.WriteLine("ICV error count                   = " & m.rsp.ICVErrCnt)

            sw.Write(vbNewLine)
            sw.WriteLine("--------------------------------------------------")  ' 40 dashes

        End Using
        HomeScreen.btn_ShowResults.Enabled = True

    End Sub

    Public Sub dumpinFile(ByVal arr As Byte())
        Dim newfilePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "dumpLog.txt")
        Using sw As StreamWriter = New StreamWriter(newfilePath)
            For Each b As Byte In arr
                sw.WriteLine(b)
            Next
        End Using
    End Sub
End Class
