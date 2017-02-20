Imports System, System.Net, System.Net.Sockets, System.Windows.Forms
Imports System.Text, System.IO, System.Threading
Imports System.Collections, System.Collections.Generic, System.ComponentModel, System.Management
Imports System.Runtime.InteropServices
'Imports Excel = Microsoft.Office.Interop.Excel

Public Class shpgpStats

    '   Filepath strings
    Public filePath As String = String.Empty
    Public lotSummaryFilePath As String = String.Empty
    Public serialtoMACFilePath As String = String.Empty
    Private o = New Object

    '
    '   RF Calibration Methods
    '
    Enum eRfCalType As Byte

        Manual
        Auto

    End Enum

#Region "Structure Declaration"

    '   <summary>
    '       
    '   </summary>
    <StructLayout(LayoutKind.Sequential, pack:=1, size:=135)> _
    Public Structure shpgpHalStats

        '   Rx Statistics counters
        Public TotalRxGoodFrmCnt As UInteger
        Public TotalRxBytesCnt As UInteger
        Public RxGoodDataCnt As UInteger
        Public RxGoodBcnCnt As UInteger
        Public RxGoodMgmtCnt As UInteger
        Public RxGoodSoundCnt As UInteger
        Public RxErrBcnCnt As UInteger
        Public BcnRxIntCnt As UInteger
        Public DuplicateRxCnt As UInteger

        '   Tx Statistics counters
        Public TotalTxFrmCnt As UInteger
        Public TotalTxBytesCnt As UInteger
        Public TxDataCnt As UInteger
        Public TxBcnCnt As UInteger
        Public TxMgmtCnt As UInteger
        Public TxDataMgmtGoodCnt As UInteger
        Public BcnSyncCnt As UInteger
        Public BcnSentIntCnt As UInteger

        '   Tx Test Stat
        Public CurTxTestFrmCnt As UInteger
        Public TxSeqNum As Byte

        '   Rx Test Stat - valid only for single tx-rx setup only 
        Public PrevRxSeqNum As Byte
        Public TotalRxMissCnt As UInteger
        Public CorruptFrmCnt As UInteger

        Public bpIntCnt As UInteger
        Public syncTestValIdx As Byte
        Public lastSs1 As UInteger
        Public MissSyncCnt As UInteger

        '   rx Phy Active stuck workaround
        Public prevBPTotalRxCnt As UInteger

        Public STAleadCCOCount As UInteger
        Public STAlagCCOCount As UInteger

        Public paRxEnHiCnt As Byte
        Public phyActHangRstCnt As Byte

        Public macTxStuckCnt As UShort
        Public macRxStuckCnt As UShort
        Public phyStuckCnt As UShort
        Public mpiRxStuckCnt As UShort
        Public smTxStuckCnt As UShort
        Public smRxStuckCnt As UShort
        Public macHangRecover1 As Byte
        Public macHangRecover2 As Byte

        Public PtoHswDropCnt As UInteger
        Public HtoPswDropCnt As UInteger
        Public GswDropCnt As UInteger

    End Structure

    '   <summary>
    '       PLC Result params
    '   </summary>
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

    '   <summary>
    '       RF Result params
    '   </summary>
    <StructLayout(LayoutKind.Sequential, pack:=1, size:=36)> _
    Public Structure _sRfStats

        'Rx Stats - Available from Receiver
        <FieldOffset(0)> <MarshalAsAttribute(UnmanagedType.U4)> Public rx_count As UInteger
        <FieldOffset(4)> <MarshalAsAttribute(UnmanagedType.U4)> Public rx_bytes As UInteger
        <FieldOffset(8)> <MarshalAsAttribute(UnmanagedType.U2)> Public decrypt_err As UShort

        'Tx Stats - Available from Transmitter
        <FieldOffset(10)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_success_count As UShort
        <FieldOffset(12)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_transaction_overflow As UShort
        <FieldOffset(14)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_transaction_expired As UShort
        <FieldOffset(16)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_channel_access_failure As UShort
        <FieldOffset(18)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_invalid_address As UShort
        <FieldOffset(20)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_invalid_gts As UShort
        <FieldOffset(22)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_no_ack As UShort
        <FieldOffset(24)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_counter_error As UShort
        <FieldOffset(26)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_frame_too_long As UShort
        <FieldOffset(28)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_unavailable_key As UShort
        <FieldOffset(30)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_unsupported_security As UShort
        <FieldOffset(32)> <MarshalAsAttribute(UnmanagedType.U2)> Public tx_invalid_parameter As UShort

        '   Calibration Result Parameters
        <FieldOffset(34)> <MarshalAsAttribute(UnmanagedType.U1)> Public rfCalAttemptCount As Byte
        <FieldOffset(35)> <MarshalAsAttribute(UnmanagedType.U1)> Public autoCalibrated As Byte

    End Structure

#End Region

    '
    '   Log PLC Test Results
    '
    Private Sub logPLCResults(ByRef m As HomeScreen.metadata, ByRef sw As StreamWriter)

        '   Append PLC TX Params
        sw.WriteLine("Parameters - ")
        sw.Write(vbNewLine)

        Dim ermode As String = String.Empty
        If m.testPramas.ermode = TestSettings.erModestate.erModeOFF Then
            ermode = "erMode OFF"
        ElseIf m.testPramas.ermode = TestSettings.erModestate.erModeON Then
            ermode = "erMode ON"
        End If

        If m.testPramas.txpowermode = TestSettings.eTxPwrMode.HIGH_TX_POWER_MODE Then
            sw.WriteLine("High Power Mode" & "   " & ermode)
        ElseIf m.testPramas.txpowermode = TestSettings.eTxPwrMode.NORMAL_TX_POWER_MODE Then
            sw.WriteLine("Normal Power Mode" & "   " & ermode)
        ElseIf m.testPramas.txpowermode = TestSettings.eTxPwrMode.AUTOMOTIVE_TX_POWER_MODE Then
            sw.WriteLine("Auto Power Mode" & "   " & ermode)
        End If

        If m.testPramas.mcstMode = TestSettings.eFrmMcstMode.HPGP_MCST Then
            sw.WriteLine("Mode = Multicast")
        ElseIf m.testPramas.mcstMode = TestSettings.eFrmMcstMode.HPGP_UCST Then
            sw.WriteLine("Mode = Unicast")
        End If

        sw.WriteLine("Frame Length = " & m.testPramas.frmLen & " Bytes" & "   " & "Inter Frame Delay = " & _
                     m.testPramas.delay & "   " & "Number of Frames = " & m.testPramas.numFrames)

        If m.testPramas.eks.ToString = "15" Then
            sw.WriteLine("Key Index = 0x0F " & "   " & "Encryption = " & _
                     CType([Enum].Parse(GetType(TestSettings.eSecTestMode), _
                                        m.testPramas.secTestMode), TestSettings.eSecTestMode).ToString)
        Else
            sw.WriteLine("Key Index = " & m.testPramas.eks.ToString & "   " & "Encryption = " & _
                     CType([Enum].Parse(GetType(TestSettings.eSecTestMode), _
                                        m.testPramas.secTestMode), TestSettings.eSecTestMode).ToString)
        End If

        sw.WriteLine("Frame Type = " & _
                     CType([Enum].Parse(GetType(TestSettings.eFrmType), m.testPramas.frmType),  _
                         TestSettings.eFrmType).ToString _
                     & "   " & "Length Test Mode = " & _
                     CType([Enum].Parse(GetType(TestSettings.eLenTestMode), _
                                        m.testPramas.lenTestMode), TestSettings.eLenTestMode).ToString)
        sw.Write(vbNewLine)

        '   Append PLC Result
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

    End Sub

    '
    '   Log RF Test Results
    '
    Private Sub logRFResults(ByRef m As HomeScreen.metadata, ByRef sw As StreamWriter)

        '   Append RF TX Params
        sw.WriteLine("Parameters - ")
        sw.Write(vbNewLine)

        sw.WriteLine("Frame Length = " & m.rftestPramas.frameLength & " Bytes" & "   " & "Inter Frame Delay = " & _
                     m.rftestPramas.interFrameDelay & "   " & "Number of Frames = " & m.rftestPramas.frameCount)

        sw.WriteLine("Channel = " & m.rftestPramas.ch.ToString("X") & "   " & "Pan ID = " & m.rftestPramas.panId.ToString("X"))
        sw.Write(vbNewLine)

        '   Append PLC Result
        sw.WriteLine("Result of the test - ")
        sw.Write(vbNewLine)
        sw.WriteLine("Total received frames             = " & m.rf_rsp.rx_count)
        sw.WriteLine("Total received bytes              = " & m.rf_rsp.rx_bytes)
        sw.WriteLine("Response decryption error         = " & m.rf_rsp.decrypt_err)
        sw.WriteLine("Calibration Results - ")
        sw.WriteLine("Calibration Attempts              = " & m.rf_rsp.rfCalAttemptCount)
        sw.WriteLine("Calibration Method                = " & CType([Enum].Parse(GetType(eRfCalType), m.rf_rsp.autoCalibrated), eRfCalType))

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
            If HomeScreen.calStatus = False Then
                sw.WriteLine("******************* RF Calibration Status *******************")
                sw.WriteLine("RF Calibration Failed. Unable to start RF Tests" & vbCrLf)
            End If
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

            If m.intf = RunTest.TestInterface.TEST_PLC_ID Then
                '   PLC Stats
                logPLCResults(m, sw)

            ElseIf m.intf = RunTest.TestInterface.TEST_802_15_5_ID Then
                '   RF Stats
                logRFResults(m, sw)

            End If

            sw.Write(vbNewLine)
            sw.WriteLine("--------------------------------------------------")  ' 40 dashes

        End Using
        HomeScreen.btn_ShowResults.Enabled = True

    End Sub

    '
    '   Write Summary in tabbed text file
    '
    Public Sub logSumary(ByVal s As HomeScreen.summary)
        Try
            Dim path As String = System.IO.Path.Combine(s.filepath, "Lot Summary.txt")
            If My.Computer.FileSystem.FileExists(path) Then

                Dim sw = New StreamWriter(path, True)

                ' Calculate how many tests are passed
                Dim pass As Integer = 0
                For Each t As HomeScreen.variations In s.tests
                    If t.result = True Then
                        pass += 1
                    End If
                Next
                Dim passStr As String = "0 " & pass.ToString & "/" & s.tests.Count.ToString
                Using sw
                    sw.WriteLine(s.serialNum & vbTab & s.MAC & vbTab & s.finalResult & vbTab & passStr)
                End Using

            Else    ' if the log file does not exist then create and write first entry

                Dim sw = New StreamWriter(path, True)

                ' Calculate how many tests are passed
                Dim pass As Integer = 0
                For Each t As HomeScreen.variations In s.tests
                    If t.result = True Then
                        pass += 1
                    End If
                Next
                Dim passStr As String = "0 " & pass.ToString & "/" & s.tests.Count.ToString
                Using sw
                    sw.WriteLine("Serial Number" & vbTab & "MAC Address" & vbTab & "Final Result" & vbTab & "Pass Tests Stats")
                    sw.WriteLine(s.serialNum & vbTab & s.MAC & vbTab & s.finalResult & vbTab & passStr)
                End Using

            End If
            
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

    '
    '   Serial number to MAC address mapping
    '
    Public Sub serialToMAC_map(ByVal s As HomeScreen.summary)

        Try
            ' If lot summary file does not exist then create the file
            s.xlfilepath = System.IO.Path.Combine(s.filepath, "Serial to MAC mapping.txt")

            If My.Computer.FileSystem.FileExists(s.xlfilepath) Then

                Dim sw = New StreamWriter(s.xlfilepath, True)
                Using sw
                    sw.WriteLine(s.serialNum & vbTab & s.MAC)
                End Using

            Else    ' if the log file does not exist then create and write first entry

                Dim sw = New StreamWriter(s.xlfilepath, True)
                Using sw
                    sw.WriteLine("Serial Number" & vbTab & "MAC Address")
                    sw.WriteLine(s.serialNum & vbTab & s.MAC)
                End Using

            End If

        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try

    End Sub

    '
    '   Release acquired object
    '
    Private Sub releaseObject(ByVal obj As Object)
        Try
            System.Runtime.InteropServices.Marshal.ReleaseComObject(obj)
            obj = Nothing
        Catch ex As Exception
            obj = Nothing
        Finally
            GC.Collect()
        End Try
    End Sub

    Public Sub dumpinFile(ByVal arr As Byte())
        Try
            SyncLock o
                Dim newfilePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "dumpLog.txt")
                Using sw As StreamWriter = New StreamWriter(newfilePath)
                    For Each b As Byte In arr
                        sw.WriteLine(b)
                    Next
                    sw.WriteLine("------------------------------------")
                End Using
            End SyncLock
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

End Class
