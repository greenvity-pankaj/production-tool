Imports System, System.Net, System.Net.Sockets, System.Windows.Forms
Imports System.Text, System.IO, System.Threading
Imports System.Collections, System.Collections.Generic, System.ComponentModel, System.Management
Imports System.Runtime.InteropServices
Imports Excel = Microsoft.Office.Interop.Excel

Public Class shpgpStats

    '   Filepath strings
    Public filePath As String = String.Empty
    Public lotSummaryFilePath As String = String.Empty
    Public serialtoMACFilePath As String = String.Empty

    '   Row Offset
    Private ROWOFFSET As Integer = 5
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
    '   load
    '
    Sub New()
        'Try
        '    filePath = System.IO.Path.Combine(My.Computer.FileSystem.SpecialDirectories.MyDocuments, "Test Results.txt")
        '    If Not System.IO.File.Exists(filePath) Then
        '        System.IO.File.Create(filePath).Dispose()
        '    End If
        'Catch fileException As Exception
        '    MessageBox.Show(fileException.ToString)
        'End Try
    End Sub

    '
    '   Log PLC Test Results
    '
    Private Sub logPLCResults(ByRef m As HomeScreen.metadata, ByRef sw As StreamWriter)

        '   Append PLC TX Params
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
    '   Create Lot summary file   
    '
    Public Sub createSummaryLog(ByVal s As HomeScreen.summary)

        ' If lot summary file does not exist then create the file
        Dim path As String = System.IO.Path.Combine(s.filepath, "Lot Summary.xls")

        If Not My.Computer.FileSystem.FileExists(path) Then

            Dim xlApp As Excel.Application = New Microsoft.Office.Interop.Excel.Application()

            If xlApp Is Nothing Then
                MessageBox.Show("Excel is not properly installed!!")
                Return
            End If

            Dim xlWorkBook As Excel.Workbook = Nothing
            Dim xlWorkSheet As Excel.Worksheet = Nothing
            Dim misValue As Object = System.Reflection.Missing.Value

            xlWorkBook = xlApp.Workbooks.Add(misValue)
            xlWorkSheet = xlWorkBook.Sheets("sheet1")

            ' Heading
            Dim chartRange As Excel.Range
            chartRange = xlWorkSheet.Range("a1", "d1")
            chartRange.Merge()
            chartRange = xlWorkSheet.Range("a2", "d2")
            chartRange.Merge()

            chartRange = xlWorkSheet.Range("a1", "d1")
            chartRange.FormulaR1C1 = "Lot 1 Summary"
            chartRange.HorizontalAlignment = 3
            chartRange.VerticalAlignment = 3
            chartRange.Font.Size = 18

            xlWorkSheet.Cells(3, 1) = "Serial Number"
            xlWorkSheet.Cells(3, 2) = "MAC Address"
            xlWorkSheet.Cells(3, 3) = "Final Result"
            xlWorkSheet.Cells(3, 4) = "Pass Tests Stats"
            xlWorkSheet.Range("3:4").EntireColumn.AutoFit()

            ' make it bold
            Dim formatRange As Excel.Range
            formatRange = xlWorkSheet.Range("a1")
            formatRange.EntireRow.Font.Bold = True
            formatRange = xlWorkSheet.Range("a3")
            formatRange.EntireRow.Font.Bold = True

            ' Fix first row
            xlWorkSheet.Activate()
            xlWorkSheet.Application.ActiveWindow.SplitRow = 4
            xlWorkSheet.Application.ActiveWindow.FreezePanes = True

            xlWorkBook.SaveAs(path, Excel.XlFileFormat.xlWorkbookNormal, misValue, misValue, misValue, misValue, _
             Excel.XlSaveAsAccessMode.xlExclusive, misValue, misValue, misValue, misValue, misValue)
            xlWorkBook.Close(True, misValue, misValue)
            xlApp.Quit()

            releaseObject(xlWorkSheet)
            releaseObject(xlWorkBook)
            releaseObject(xlApp)

            MessageBox.Show("Excel file created , you can find the file " & path)
        End If

    End Sub

    '
    '   Write Summary in Excel file
    '
    Public Sub logSumary(ByVal s As HomeScreen.summary)
        Try
            Dim path As String = System.IO.Path.Combine(s.filepath, "Lot Summary.xls")

            Dim xlApp As Excel.Application = New Microsoft.Office.Interop.Excel.Application()
            If xlApp Is Nothing Then
                MessageBox.Show("Excel is not properly installed!!")
                Return
            End If

            Dim xlWorkBook As New Excel.Workbook
            xlWorkBook = xlApp.Workbooks.Open(path)

            Dim xlWorkSheet As Excel.Worksheet
            xlWorkSheet = xlWorkBook.Sheets("sheet1")

            '   Fill values
            xlWorkSheet.Cells(ROWOFFSET, 1) = s.serialNum
            xlWorkSheet.Cells(ROWOFFSET, 2) = s.MAC
            xlWorkSheet.Cells(ROWOFFSET, 3) = s.finalResult

            Dim pass As Integer = 0
            For Each t As HomeScreen.variations In s.tests
                If t.result = True Then
                    pass += 1
                End If
            Next
            xlWorkSheet.Cells(ROWOFFSET, 4) = pass.ToString & "/" & s.tests.Count.ToString

            ' ready ROWOFFSET for next iteration
            ROWOFFSET += 1

            xlWorkBook.SaveAs(path)
            xlWorkBook.Close(True)
            xlApp.Quit()

            releaseObject(xlWorkSheet)
            releaseObject(xlWorkBook)
            releaseObject(xlApp)
        Catch ex As Exception
            MessageBox.Show(ex.ToString)
        End Try
    End Sub

    '
    '
    '
    Private Sub create_serialToMAC_file(ByRef s As HomeScreen.summary)
        ' If lot summary file does not exist then create the file
        s.xlfilepath = System.IO.Path.Combine(s.filepath, "Serial to MAC mapping.xls")

        If Not My.Computer.FileSystem.FileExists(s.xlfilepath) Then

            Dim xlApp As Excel.Application = New Microsoft.Office.Interop.Excel.Application()

            If xlApp Is Nothing Then
                MessageBox.Show("Excel is not properly installed!!")
                Return
            End If

            Dim xlWorkBook As Excel.Workbook = Nothing
            Dim xlWorkSheet As Excel.Worksheet = Nothing
            Dim misValue As Object = System.Reflection.Missing.Value

            xlWorkBook = xlApp.Workbooks.Add(misValue)
            xlWorkSheet = xlWorkBook.Sheets("sheet1")

            xlWorkSheet.Cells(1, 1) = "Serial Number"
            xlWorkSheet.Cells(1, 2) = "MAC Address"

            xlWorkBook.SaveAs(s.xlfilepath, Excel.XlFileFormat.xlWorkbookNormal, misValue, misValue, misValue, misValue, _
             Excel.XlSaveAsAccessMode.xlExclusive, misValue, misValue, misValue, misValue, misValue)
            xlWorkBook.Close(True, misValue, misValue)
            xlApp.Quit()

            releaseObject(xlWorkSheet)
            releaseObject(xlWorkBook)
            releaseObject(xlApp)

            MessageBox.Show("Excel file created , you can find the file " & s.xlfilepath)
        End If
    End Sub

    '
    '   Serial number to MAC address mapping
    '
    Dim macRowOffset As Integer = 3
    Public Sub serialToMAC_map(ByVal s As HomeScreen.summary)
        create_serialToMAC_file(s)

        Try
            Dim xlApp As Excel.Application = New Microsoft.Office.Interop.Excel.Application()
            If xlApp Is Nothing Then
                MessageBox.Show("Excel is not properly installed!!")
                Return
            End If

            Dim xlWorkBook As New Excel.Workbook
            xlWorkBook = xlApp.Workbooks.Open(s.xlfilepath)

            Dim xlWorkSheet As Excel.Worksheet
            xlWorkSheet = xlWorkBook.Sheets("sheet1")

            '   Fill values
            xlWorkSheet.Cells(macRowOffset, 1) = s.serialNum
            xlWorkSheet.Cells(macRowOffset, 2) = s.MAC

            ' ready ROWOFFSET for next iteration
            macRowOffset += 1

            xlWorkBook.SaveAs(s.xlfilepath)
            xlWorkBook.Close(True)
            xlApp.Quit()

            releaseObject(xlWorkSheet)
            releaseObject(xlWorkBook)
            releaseObject(xlApp)
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

    Private o = New Object
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
