Imports System.IO.Ports
Imports System.IO 'for serial port control 
Imports System.Threading ' for threading and to supress cross thread warning and error
Imports System.Xml 'for xml parsing
Imports System.Text ' for stringbuilder class

'Imports System.Diagnostics
'Imports System.Runtime.InteropServices.Marshal


Public Class Form1


    Declare Function SendMessage Lib "user32" Alias "SendMessageA" (ByVal hwnd As Integer, ByVal wMsg As Integer, ByVal wParam As Integer, ByVal lParam As Integer) As Integer

    Private Declare Function GetScrollPos Lib "user32.dll" ( _
           ByVal hWnd As IntPtr, _
           ByVal nBar As Integer) As Integer

    Private Declare Function SetScrollPos Lib "user32.dll" ( _
            ByVal hWnd As IntPtr, _
            ByVal nBar As Integer, _
            ByVal nPos As Integer, _
            ByVal bRedraw As Boolean) As Integer

    Private Declare Function PostMessageA Lib "user32.dll" ( _
            ByVal hwnd As IntPtr, _
            ByVal wMsg As Integer, _
            ByVal wParam As Integer, _
            ByVal lParam As Integer) As Boolean

    Public Declare Function GetScrollInfo Lib "user32" Alias "GetScrollInfo" ( _
            ByVal hWnd As IntPtr, _
            ByVal nBar As Integer, _
            ByRef lpScrollInfo As SCROLLINFO) As Integer

    Dim filepath As String
    Dim portopen As Boolean = False
    Dim hex_file As String
    Dim serialString As String
    Dim position As Integer = 0
    Dim cmdEraseFlash As Integer = 0
    Dim bankId As Integer = -1 ' update this bank id from serial receiver handler
    Dim currentBankId As Integer = 0 ' update this in auto flash thread
    Dim errorsignal As Integer = 0
    Dim errorsig As Integer = 0

    Dim hexFileExtensionHex As Byte = 0

    Dim j As Integer = 0 ' Used to maintain bank count in for loop
    Dim stringGreenvity As String = "**     GREENVITY COMMUNICATIONS INC        **"
    Dim stringBootloader As String = "**          Boot loader V2.0               **"
    Dim stringBootloader3 As String = "Boot loader V3.0"
    Dim stringProgramFlash As String = " --> Program SFLASH Y/N? :"
    Dim stringEraseFlash As String = " --> Delete current code Y/N? :"

    Dim stringCommonId As String = "##### Download code for - Common #####"
    Dim stringBankId0 As String = " ##### Download code for - BANK-00 #####"
    Dim stringBankId1 As String = " ##### Download code for - BANK-01 #####"
    Dim stringBankId2 As String = " ##### Download code for - BANK-02 #####"
    Dim stringBankId3 As String = " ##### Download code for - BANK-03 #####"
    Dim stringBankId4 As String = " ##### Download code for - BANK-04 #####"
    Dim stringBankId5 As String = " ##### Download code for - BANK-05 #####"
    Dim stringBankId6 As String = " ##### Download code for - BANK-06 #####"
    Dim stringBankId7 As String = " ##### Download code for - BANK-07 #####"

    Dim stringBankCramId0 As String = "Download code from UART to CRAM - BANK-00"
    Dim stringBankCramId1 As String = "Download code from UART to CRAM - BANK-01"
    Dim stringBankCramId2 As String = "Download code from UART to CRAM - BANK-02"
    Dim stringBankCramId3 As String = "Download code from UART to CRAM - BANK-03"
    Dim stringBankCramId4 As String = "Download code from UART to CRAM - BANK-04"
    Dim stringBankCramId5 As String = "Download code from UART to CRAM - BANK-05"
    Dim stringBankCramId6 As String = "Download code from UART to CRAM - BANK-06"
    Dim stringBankCramId7 As String = "Download code from UART to CRAM - BANK-07"

    Dim stringWaitingForIntelHexFile As String = "--> Waiting for Intel-hex file ."
    Dim stringWritingToSFlash As String = " --> Writing to sflash "
    Dim stringSummery As String = " --> Code Download Summary"
    Dim stringSuccessDownloadBytes As String = " .Successfully downloaded byte(s): "
    Dim stringFoundLines As String = " .Found line(s): "
    Dim stringFalseLines As String = " .False line(s): "
    Dim stringRecordTypeError As String = " .Record type error(s): "
    Dim stringNonAsciiDigits As String = " .Non ascii digit(s): "
    Dim stringErrorDownloadBytes As String = " .Error downloaded byte(s): "
    Dim stringFailedChecksume As String = " .Failed checksume(s): "
    Dim stringSFlashProgrammingError As String = "*** SFLASH programming error"
    Dim stringErrorUARTToRAM As String = "*** ERROR downloading from UART to RAM"

    Dim stringRequestResetChar As String = "--> Press reset or hit 's' to reboot the system"
    Dim stringRunningFw As String = "--> Running firmware"

    Const stringBootLoaderFileName As String = "bootld_8051.H00"

    Dim stringCommonAddress As String = ":102100"
    Dim bank_address_ucase As String = ":10A0000"
    Dim bank_address_lcase As String = ":10a0000"
    Dim stringExtLinAddressRecord As String = ":02000004"
    Dim stringEofRecord As String = ":00000001FF" 'end of record is used to identify end of file and to skip flashing of any bank
    Dim bank_file(8 + 1) As String ' This has splitted and optimized hex files if file with .hex extension is selected 
    Dim bank_file_common As String
    Dim nonNumberEntered As Boolean = False
    Dim backspaceCheck As Boolean = False

    Const BANK_ID_MAX = 7 ' Bank ID starts from 0 to 7. Total 8 Banks

    Const DATA_RECORD = 0
    Const EOF_RECORD = 1
    Const EXT_SEG_RECORD = 2
    Const EXT_LIN_ADDR_RECORD = 4

    Const LOAD_IDLE = 0
    Const LOAD_BOOT = 1
    Const LOAD_FW = 2

    Dim flash_state_cr As Byte = LOAD_IDLE
    ' Graphics related variables
    Dim form_size As Size
    Dim flash_mode_size As Size
    Dim tabPage1_Size As Size
    Dim grbx_flashOption_Location As Point
    Dim tab_ctrl_flashmode_Location As Point
    Dim grbx_console_Location As Point

    ' Scrollbar direction
    '
    Const SBS_HORZ = 0
    Const SBS_VERT = 1

#If FEATURE = "FLASH" Then
    Const PRODUCT_TITLE As String = "Load on Flash Utility "
#ElseIf FEATURE = "PROD" Then
    Const PRODUCT_TITLE As String = "Load on RAM Utility "
#ElseIf FEATURE = "DEVELOPER" Then
    Const PRODUCT_TITLE As String = "Flashing Utility GV701xB2-CRLC "

#Else
    Const PRODUCT_TITLE As String = "Flashing Utility "
#End If
    ' Windows Messages

    Const WM_VSCROLL = &H115
    Const WM_HSCROLL = &H114
    Const SB_THUMBPOSITION = 4

    Public Structure SCROLLINFO
        Public cbSize As Integer
        Public fMask As Integer
        Public nMin As Integer
        Public nMax As Integer
        Public nPage As Integer
        Public nPos As Integer
        Public nTrackPos As Integer
    End Structure

    Private Enum ScrollInfoMask
        SIF_RANGE = &H1
        SIF_PAGE = &H2
        SIF_POS = &H4
        SIF_DISABLENOSCROLL = &H8
        SIF_TRACKPOS = &H10
        SIF_ALL = SIF_RANGE + SIF_PAGE + SIF_POS + SIF_TRACKPOS
    End Enum

    Private Enum ScrollBarDirection
        SB_HORZ = 0
        SB_VERT = 1
    End Enum

    Const EM_LINESCROLL As Integer = &HB6

    Public Structure rf_param_t
        Dim reg_23 As Byte
        Dim reg_24 As Byte
        Dim channel As Byte
        Dim channel_mask As Int32
    End Structure

    Public Structure deviceCap_t
        'u8        ccoCap :            2;    //CCo capability level
        'u8        proxyNetCap :       1;    //proxy networking capability
        'u8        backupCcoCap :      1;    //backup Cco capability
        'u8        Rsvd :              4;
        Dim ccoCap As Byte
        Dim greenPhyCap As Byte
        Dim powerSaveCap As Byte
        Dim repeaterRouting As Byte
        Dim HPAVVersion As Byte
        Dim bridgeSupported As Byte
    End Structure

    Public Structure hexRecord_t
        Dim record_valid As Integer
        Dim no_of_bytes As Integer
        Dim address As Integer
        Dim record_type As Integer
        Dim ext_linear_address As Integer
    End Structure


    'Const MAX_SYSTEM_NAME = 32
    'Const MAC_ADDR_LEN = 6
    'Const VER_SIZE = 20
    'Const MAX_DPW_LEN = 32
    'Const ENC_KEY_LEN = 16
    'Const NID_LEN = 7
    'Const IEEE_MAC_ADDRESS = 8
    'Const PLC_CALIBRATION_LEN = 20
    'Const PLC_BASEBAND_PARAM_LENGTH = 20


    'Public Structure _sysProfile_t
    '    Dim systemName() As Byte
    '    Dim macAddress() As Byte
    '    Dim swVersion() As Byte

    '    'u8 ethPortEn :   1;
    '    'u8 spiPort :     1;
    '    'u8 lineMode :   1;  //AC Or DC
    '    'u8 lineFreq :   1;  // 50 Or 60
    '    'u8 devMode :        2;       // Auto, STA, CCo	
    '    'u8 lastdevMode :    2;
    '    Dim rsvd As Byte
    '    Dim lastUserAppCCOState As Byte
    '    Dim secLevel As Byte
    '    Dim ukeEnable As Byte

    '    Dim powerSaveMode As Byte ' //Disable, Short_Sleep, Max_PS  2 Bits
    '    Dim advPowerSaveMode As Byte ' //Wake Time, Sleep Time
    '    Dim cap As deviceCap_t
    '    Dim devicePassword() As Byte
    '    Dim UKEEn As Byte
    '    Dim nmk() As Byte

    '    Dim nid() As Byte
    '    Dim zigbeePortEn As Byte
    '    Dim zigbeeAddr() As Byte
    '    Dim plcCalibrationParam() As Byte
    '    Dim plcBaseBandParam() As Byte

    '    Dim rfParam As rf_param_t

    'End Structure

    Private Sub filepath_sub()
        Dim pos, pos1 As Integer
        Dim filenamepath As String
        Dim filename As String
        Dim extension As String

        'If (txtbx_filePath.Text <> vbNullString) Then
        OpenFileDialog1.InitialDirectory = txtbx_filePath.Text
        ' If (OpenFileDialog1.CheckPathExists()) Then
        If OpenFileDialog1.ShowDialog = DialogResult.OK Then

            'filepath = FolderBrowserDialog1.SelectedPath
            filenamepath = OpenFileDialog1.FileName

            pos = filenamepath.LastIndexOf(".")
            extension = UCase(Strings.Right(filenamepath, (filenamepath.Length - pos - 1)))
            ' UCASE reduces double comparision of HEX and hex extension

            If (extension = "HEX") Then
                pos = filenamepath.LastIndexOf("\")
                filepath = filenamepath.Remove(pos, filenamepath.Length - pos)
                txtbx_filePath.Text = filepath
                filename = filenamepath.Remove(0, pos + 1)
                txtbx_fileName.Text = filename
                hexFileExtensionHex = 1 ' Helps to identify file splitting and post processing required
            Else
                pos = filenamepath.LastIndexOf("\")
                filepath = filenamepath.Remove(pos, filenamepath.Length - pos)
                txtbx_filePath.Text = filepath
                filename = filenamepath.Remove(0, pos + 1)
                pos1 = filename.LastIndexOf(".")
                txtbx_fileName.Text = filename.Remove(pos1, filename.Length - pos1)
                hexFileExtensionHex = 0 '' Helps to identify file splitting and post processing not required
            End If
        End If
        'End If
        'End If
    End Sub

    Function record_parse(ByVal datafile As String, ByVal record_index As Integer) As hexRecord_t
        Dim hex_record_fields As hexRecord_t
        Dim temp_char As String

        If (record_index >= 0) Then
            If (datafile.Chars(record_index) = ":") Then

                temp_char = Mid(datafile, record_index + 2, 2)
                hex_record_fields.no_of_bytes = CLng("&H" & temp_char)

                temp_char = Mid(datafile, record_index + 4, 4)
                hex_record_fields.address = CLng("&H" & temp_char)

                temp_char = Mid(datafile, record_index + 8, 2)
                hex_record_fields.record_type = CLng("&H" & temp_char)

                If (hex_record_fields.record_type = DATA_RECORD) Then

                ElseIf (hex_record_fields.record_type = EOF_RECORD)

                ElseIf (hex_record_fields.record_type = EXT_SEG_RECORD)
                    'MsgBox("extended segment record found")
                ElseIf (hex_record_fields.record_type = EXT_LIN_ADDR_RECORD)
                    temp_char = Mid(datafile, record_index + 10, 4)
                    hex_record_fields.ext_linear_address = CLng("&H" & temp_char)
                End If

            End If
            hex_record_fields.record_valid = 1
        Else
            hex_record_fields.record_valid = -1
        End If

        Return hex_record_fields
    End Function

    Sub hex_editor_sub(ByVal hex_file As String)
        Dim i As Integer

        Dim total_no_of_banks_hex_record As Integer = 0
        Dim j As Integer
        Dim position As Integer
        Dim record_index As Integer = 0
        Dim next_record_index As Integer = 0

        Dim hex_record_fields As hexRecord_t
        'Dim txtbxbuilder As New StringBuilder 'remove during cleanup

        Dim bank_id As Byte = 0
        position = 0
        j = 0
        '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
        record_index = hex_file.LastIndexOf(stringExtLinAddressRecord)
        hex_record_fields = record_parse(hex_file, record_index)
        If (hex_record_fields.record_valid > 0) Then
            If (hex_record_fields.record_type = EXT_LIN_ADDR_RECORD) Then
                total_no_of_banks_hex_record = hex_record_fields.ext_linear_address
            End If
        End If

        For j = 0 To total_no_of_banks_hex_record
            Dim builder As New StringBuilder 'New initializes always
            record_index = hex_file.IndexOf(stringExtLinAddressRecord, position)
            hex_record_fields = record_parse(hex_file, record_index)

            If (hex_record_fields.record_valid > 0) Then
                bank_id = hex_record_fields.ext_linear_address
            End If

            If (bank_id = 0) Then

                record_index = hex_file.IndexOf(stringCommonAddress, record_index)
                If (record_index >= 0) Then
                    next_record_index = hex_file.IndexOf(bank_address_ucase, record_index)
                    If (next_record_index >= 0) Then
                        For i = record_index To next_record_index - 1
                            builder.Append(hex_file.Chars(i))
                        Next
                        builder.Append(stringEofRecord & vbCrLf)
                        bank_file_common = builder.ToString
                    End If
                    position = next_record_index
                End If

                Dim builder1 As New StringBuilder 'New initializes always
                record_index = next_record_index 'hex_file.IndexOf(bank_address_ucase, record_index)
                If (record_index >= 0) Then
                    next_record_index = hex_file.IndexOf(stringExtLinAddressRecord, record_index)
                    If (next_record_index >= 0) Then
                        bank_file(bank_id) = ""
                        For i = record_index To next_record_index - 1
                            builder1.Append(hex_file.Chars(i))
                        Next

                        builder1.Append(stringEofRecord & vbCrLf)
                        bank_file(bank_id) = builder1.ToString
                    Else
                        next_record_index = hex_file.IndexOf(stringEofRecord, record_index)

                        If (next_record_index >= 0) Then
                            bank_file(bank_id) = ""
                            For i = record_index To next_record_index - 1
                                builder1.Append(hex_file.Chars(i))
                            Next
                            builder1.Append(stringEofRecord & vbCrLf)
                            bank_file(bank_id) = builder1.ToString
                        End If
                    End If
                    position = next_record_index
                Else
                    record_index = hex_file.IndexOf(bank_address_lcase, record_index)
                    If (record_index >= 0) Then
                    End If
                    position = record_index
                End If

            Else
                record_index = hex_file.IndexOf(bank_address_ucase, record_index)
                If (record_index >= 0) Then
                    next_record_index = hex_file.IndexOf(stringExtLinAddressRecord, record_index)
                    If (next_record_index >= 0) Then
                        bank_file(bank_id) = ""
                        For i = record_index To next_record_index - 1
                            builder.Append(hex_file.Chars(i))
                        Next

                        builder.Append(stringEofRecord & vbCrLf)
                        bank_file(bank_id) = builder.ToString
                    Else
                        next_record_index = hex_file.IndexOf(stringEofRecord, record_index)

                        If (next_record_index >= 0) Then
                            bank_file(bank_id) = ""
                            For i = record_index To next_record_index - 1
                                builder.Append(hex_file.Chars(i))
                            Next
                            builder.Append(stringEofRecord & vbCrLf)
                            bank_file(bank_id) = builder.ToString
                        End If
                    End If
                    position = next_record_index
                Else
                    record_index = hex_file.IndexOf(bank_address_lcase, record_index)
                    If (record_index >= 0) Then
                    End If
                    position = record_index
                End If

            End If
            'txtbxbuilder.Append(vbCrLf & bank_file(bank_id))
        Next
        If (BANK_ID_MAX > total_no_of_banks_hex_record) Then
            For i = (total_no_of_banks_hex_record + 1) To BANK_ID_MAX
                bank_file(i) = stringEofRecord & vbCrLf
            Next
        End If
        'TextBox1.Text = txtbxbuilder.ToString
    End Sub


    Private Sub Form1_Load(sender As Object, e As System.EventArgs) Handles Me.Load
        Dim procid
        'SendMessage(ProgressBar1.Handle, 1046, 2, 0)
        Dim pos As Integer
        Dim xml_filenamepath As String
        Dim xml_filepath As String

        'Thread.Sleep(3000)
        'Dim screenWidth As Integer = Screen.PrimaryScreen.Bounds.Width
        'Dim screenHeight As Integer = Screen.PrimaryScreen.Bounds.Height
        'MsgBox(screenWidth & " " & screenHeight)

        form_size = Me.Size
        flash_mode_size = tab_ctrl_flashmode.Size
        tabPage1_Size = tabPage1.Size
        grbx_flashOption_Location = grbx_flashOption.Location
        tab_ctrl_flashmode_Location = tab_ctrl_flashmode.Location
        grbx_console_Location = grbx_console.Location

#If FEATURE = "FLASH" Then
        MenuStrip1.BackColor = Color.MediumOrchid
         chk_bx_load_to_cram.Checked = False
         chk_bx_load_to_cram.Enabled = False
         tab_config.Enabled = True
#ElseIf FEATURE = "PROD" Then
        MenuStrip1.BackColor = Color.LimeGreen
         chk_bx_load_to_cram.Checked = True
         chk_bx_load_to_cram.Enabled = False
        tab_config.Enabled = False
#ElseIf FEATURE = "DEVELOPER" Then
        MenuStrip1.BackColor = SystemColors.Control
        chk_bx_load_to_cram.Checked = False
        chk_bx_load_to_cram.Enabled = True
        tab_config.Enabled = True
#End If
        xml_filenamepath = Path.GetFullPath(Application.ExecutablePath)

        pos = xml_filenamepath.LastIndexOf("\")
        xml_filepath = xml_filenamepath.Remove(pos, xml_filenamepath.Length - pos)

        If IO.File.Exists(xml_filepath & "\" & "settings.xml") = True Then
            load_path()
        Else
            save_path()
        End If
        For Each sp As String In My.Computer.Ports.SerialPortNames
            combobx_portNo.Items.Add(sp)
        Next

        If combobx_portNo.Items.Count = 0 Then
            MsgBox("Serial Port not found", MsgBoxStyle.Information)
        Else
            combobx_portNo.SelectedIndex = 0
        End If
        procid = Process.GetCurrentProcess.Id
        Me.Text = PRODUCT_TITLE & Application.ProductVersion & " - PID: " & procid.ToString & " - Comm " & "Disconnected" & " - File: " & txtbx_fileName.Text

    End Sub

    Private Sub txtbx_console_KeyPress(sender As Object, e As System.Windows.Forms.KeyPressEventArgs) Handles txtbx_console.KeyPress

        If SerialPort1.IsOpen = True Then
            '        If (e.KeyChar = Keys.Control AndAlso e.KeyChar = Keys.V) Then
            If nonNumberEntered = False Then
                SerialPort1.Write(e.KeyChar)
            End If

        End If
            e.Handled = True
    End Sub

    Private Sub btn_serialConnect_Click(sender As System.Object, e As System.EventArgs) Handles btn_serialConnect.Click
        portconnect_sub()
    End Sub
    Private Sub PortConnectToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles PortConnectToolStripMenuItem.Click
        portconnect_sub()
    End Sub
    Private Sub portconnect_sub()
        Dim procid = Process.GetCurrentProcess.Id
        If portopen = False Then

            If SerialPort1.IsOpen Then
                SerialPort1.Close()
            End If

            Try
                With SerialPort1
                    .PortName = combobx_portNo.Text
                    .BaudRate = combobx_baudRate1.Text
                    .Handshake = Handshake.None
                    .Parity = Parity.None
                    .DataBits = 8
                    .StopBits = StopBits.One

                    If chkbx_rts.Checked = True Then
                        .DtrEnable = False
                    End If

                    SerialPort1.Open()
                    portopen = True
                    Me.Text = PRODUCT_TITLE & Application.ProductVersion & " - PID: " & procid.ToString & " - Connected to " & SerialPort1.PortName.ToString & " - File: " & txtbx_fileName.Text
                    btn_serialConnect.Text = "Disconnect"
                    PortConnectToolStripMenuItem.Text = "Port Disconnect"
                End With
            Catch ex As Exception
                MsgBox("Error:" & ex.ToString, MsgBoxStyle.Critical)
            End Try
        Else
            If SerialPort1.IsOpen Then
                SerialPort1.Close()
            End If
            portopen = False
            Me.Text = PRODUCT_TITLE & Application.ProductVersion & " - PID: " & procid.ToString & " - Comm " & "Disconnected" & " - File: " & txtbx_fileName.Text
            btn_serialConnect.Text = "Connect"
            PortConnectToolStripMenuItem.Text = "Port Connect"
        End If
    End Sub
    Private Sub SerialPort1_DataReceived(sender As Object, e As System.IO.Ports.SerialDataReceivedEventArgs) Handles SerialPort1.DataReceived
        Control.CheckForIllegalCrossThreadCalls = False
        Dim temp_char As String
        Dim bytecount As Integer
        'System.Threading.Thread.Sleep(50)
        If e.EventType = SerialData.Chars Then
            Do
                Try
                    bytecount = SerialPort1.BytesToRead
                    If bytecount = 0 Then
                        Exit Do
                    End If
                Catch ex As Exception
                    Exit Do
                End Try
                Dim bytebuffer(bytecount) As Byte
                Try
                    SerialPort1.Read(bytebuffer, 0, bytecount)
                    backspaceCheck = False
                    temp_char = ""
                    temp_char = System.Text.Encoding.ASCII.GetString(bytebuffer, 0, bytecount) ' this is for flash processing

                    Debug.Print(temp_char)
#If 1 Then
                    For i As Integer = 0 To bytecount - 1
                        If (bytebuffer(i) = 8) Then
                            backspaceCheck = True
                            Exit For
                        End If
                    Next
#End If
                    If backspaceCheck = True Then
                        For i As Integer = 0 To bytecount - 1

                            If (bytebuffer(i) <> 8) Then ' Handles backspace key character or stroke, keys.back
                                ' temp_char = temp_char & System.Text.Encoding.ASCII.GetChars(bytebuffer, i, 1)
                                txtbx_console.AppendText(System.Text.Encoding.ASCII.GetChars(bytebuffer, i, 1))
                                'TextBox1.AppendText((bytebuffer(i).ToString))
                            Else

                                If txtbx_console.TextLength > 0 Then
                                    'txtbx_console.Text = txtbx_console.Text.Remove(txtbx_console.TextLength - 1, 1) ' removes last character as action of backspace
                                    txtbx_console.SelectionStart = txtbx_console.TextLength - 1
                                    txtbx_console.SelectionLength = 1
                                    txtbx_console.ReadOnly = False
                                    txtbx_console.SelectedText = ""
                                    txtbx_console.ReadOnly = True
                                End If
                            End If
                        Next
                        backspaceCheck = False

                    Else
                        txtbx_console.AppendText(temp_char)
                    End If

                    'txtbx_console.AppendText(temp_char)
                    If cmdEraseFlash = 0 Then
                        serialString = serialString & temp_char
                        If (flash_state_cr = LOAD_FW) Then
                            If (serialString.IndexOf(stringRunningFw, position) >= 0) Then
                                ' position = position + stringBankId7.Length + stringWaitingForIntelHexFile.Length

                                serialString = ""
                                position = 0
                                'MsgBox("Run FW detected")
                                Try
                                    With SerialPort1
                                        .BaudRate = combobx_baudRate2.Text
                                    End With
                                Catch ex As Exception
                                    MsgBox(ex.ToString)
                                End Try
                            ElseIf (serialString.IndexOf(stringBootloader3, position) >= 0) Then
                                position = 0
                                autoflash_sub()
                            End If
                        ElseIf (flash_state_cr = LOAD_IDLE) Then
                            If (serialString.IndexOf(stringRequestResetChar, position) >= 0) Then
                                position = 0
                                serialString = ""
                                Try
                                    With SerialPort1
                                        .Write("s")
                                        Debug.Print("Reset send s")
                                    End With
                                Catch ex As Exception
                                    MessageBox.Show("Serial Port not available/disconnected")
                                End Try

                                Thread.Sleep(100)
                                Try
                                        With SerialPort1
                                            .BaudRate = combobx_baudRate1.Text
                                            Debug.Print("restore baudrate flash done")
                                        End With
                                    Catch ex As Exception
                                        MessageBox.Show("Serial Port not available/disconnected")
                                    End Try
                                End If
                                'autoflash_sub()
                            End If
                        End If
                    If cmdEraseFlash = 1 Then
                        serialString = serialString & temp_char
                        If (serialString.IndexOf(stringGreenvity, position) >= 0) And (serialString.IndexOf(stringBootloader, position) >= 0) Then
                            position = position + stringGreenvity.Length + stringBootloader.Length
                            serialString = ""
                            position = 0
                            bankId = -1
                            currentBankId = 0
                            ProgressBar1.Value = 0
                            j = 0

                            Try
                                With SerialPort1
                                    .Write("F")
                                    Debug.Print("Tx F")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try
                        ElseIf (serialString.IndexOf(stringProgramFlash, position) >= 0) Then
                            position = position + stringProgramFlash.Length + 1 ' + 1 for vbcrlf
                            serialString = ""
                            position = 0
                            Try
                                With SerialPort1
                                    .Write("Y")
                                    Debug.Print("stringProgramFlash: Tx Y")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try
                        ElseIf (serialString.IndexOf(stringEraseFlash, position) >= 0) Then
                            position = position + stringEraseFlash.Length + 1 ' + 1 for vbcrlf
                            serialString = ""
                            position = 0
                            Try
                                With SerialPort1
                                    .Write("Y")
                                    Debug.Print("stringEraseFlash: Tx Y")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try



                        ElseIf (serialString.IndexOf(stringCommonId, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringCommonId.Length + stringWaitingForIntelHexFile.Length + 3 '\n\n \n
                            serialString = ""
                            position = 0

                            bankId = 8
                            'ProgressBar1.ForeColor = Color.DarkRed
                            'SendMessage(ProgressBar1.Handle, 1040, 1, 0)
                            ProgressBar1.Value = 0
                            Debug.Print("Bank id common flash")



                            'For i = 0 To (hex_file.Length - 1)
                            Try
                                Debug.Print("Bank id write" & "Common")
                                Debug.Print(bank_file_common)
                                SerialPort1.Write(bank_file_common, 0, bank_file_common.Length)

                            Catch ex As Exception
                                MsgBox("Error: " & ex.ToString, MsgBoxStyle.Critical)

                                btn_autoFlash.Enabled = True
                                btn_abort.Enabled = False
                                ShrinkToolStripMenuItem_autoflash.Enabled = True
                                AbortToolStripMenuItem_abort.Enabled = False
                                bankId = -1
                                currentBankId = 0
                                cmdEraseFlash = 0
                                position = 0
                                serialString = ""
                                j = 0
                            End Try

                        ElseIf (serialString.IndexOf(stringBankId0, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId0.Length + stringWaitingForIntelHexFile.Length + 3 '\n\n \n
                            serialString = ""
                            position = 0

                            bankId = 0
                            'ProgressBar1.ForeColor = Color.DarkRed
                            'SendMessage(ProgressBar1.Handle, 1040, 1, 0)
                            ProgressBar1.Value = 0
                            Debug.Print("Bank id 0 flash")
                        ElseIf (serialString.IndexOf(stringBankId1, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId1.Length + stringWaitingForIntelHexFile.Length
                            serialString = ""
                            position = 0

                            bankId = 1
                            ProgressBar1.Value = 10
                            Debug.Print("Bank id 1 flash")
                        ElseIf (serialString.IndexOf(stringBankId2, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId2.Length + stringWaitingForIntelHexFile.Length
                            serialString = ""
                            position = 0

                            bankId = 2
                            'MsgBox("isr")
                            ProgressBar1.Value = 20
                            Debug.Print("Bank id 2 flash")
                        ElseIf (serialString.IndexOf(stringBankId3, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId3.Length + stringWaitingForIntelHexFile.Length
                            serialString = ""
                            position = 0

                            bankId = 3
                            ProgressBar1.Value = 30
                            Debug.Print("Bank id 3 flash")
                        ElseIf (serialString.IndexOf(stringBankId4, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId4.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0

                            bankId = 4
                            ProgressBar1.Value = 40
                            Debug.Print("Bank id 4 flash")
                        ElseIf (serialString.IndexOf(stringBankId5, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId5.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0

                            bankId = 5
                            ProgressBar1.Value = 50
                            Debug.Print("Bank id 5 flash")
                        ElseIf (serialString.IndexOf(stringBankId6, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId6.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0

                            bankId = 6
                            ProgressBar1.Value = 60
                            Debug.Print("Bank id 6 flash")
                        ElseIf (serialString.IndexOf(stringBankId7, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId7.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0

                            bankId = 7
                            ProgressBar1.Value = 70
                            Debug.Print("Bank id 7 flash")
                            'ElseIf (serialString.IndexOf(stringRunningFw, position) >= 0) Then
                            'position = position + stringBankId7.Length + stringWaitingForIntelHexFile.Length

                            ' serialString = ""
                            'position = 0
                            'MsgBox("Run FW detected")
                            'Try
                            '    With SerialPort1
                            '        .BaudRate = combobx_baudRate2.Text
                            '    End With
                            'Catch ex As Exception

                            'End Try

                            'If (flash_state_cr = LOAD_FW) Then
                        ElseIf (serialString.IndexOf(stringRunningFw, position) >= 0) Then
                            ' position = position + stringBankId7.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0
                            Debug.Print("Running fw found")
                            'MsgBox("Run FW detected")

                            If (flash_state_cr = LOAD_FW) Then
                                Try
                                    With SerialPort1
                                        .BaudRate = combobx_baudRate2.Text
                                        Debug.Print("Config Baudrate 2 done")
                                    End With
                                Catch ex As Exception
                                    MsgBox(ex.ToString)
                                End Try
                            End If
                        ElseIf (serialString.IndexOf(stringBootloader3, position) >= 0) Then
                            position = 0
                            serialString = ""
                            If (flash_state_cr = LOAD_FW) Then
                                autoflash_sub()
                                bankId = -1
                                currentBankId = 0
                                ProgressBar1.Value = 0
                                j = 0

                                Try
                                    With SerialPort1
                                        .Write("F")
                                        Debug.Print("stringBootloader3: Tx F")
                                    End With
                                Catch ex As Exception
                                    MessageBox.Show("Serial Port not available/disconnected")
                                End Try
                            End If


                            '  End If

                            'ProgressBar1.Value = 70
                            ' stringRebootSystem
                        ElseIf (serialString.IndexOf(stringRequestResetChar, position) >= 0) Then
                            position = 0
                            serialString = ""
                            Try
                                With SerialPort1
                                    .Write("s")
                                    Debug.Print("Reset send s")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try

                            If (flash_state_cr = LOAD_IDLE) Then
                                Try
                                    With SerialPort1
                                        .BaudRate = combobx_baudRate1.Text
                                        Debug.Print("restore baudrate flash done")
                                    End With
                                Catch ex As Exception
                                    MessageBox.Show("Serial Port not available/disconnected")
                                End Try
                            End If
                        ElseIf ((serialString.IndexOf(stringWritingToSFlash, position) >= 0)) Then
                            position = position + stringWritingToSFlash.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringSummery, position) >= 0)) Then
                            position = position + stringSummery.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringSuccessDownloadBytes, position) >= 0)) Then
                            position = position + stringSuccessDownloadBytes.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringFoundLines, position) >= 0)) Then
                            position = position + stringFoundLines.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringFalseLines, position) >= 0)) Then
                            position = position + stringFalseLines.Length + 1 ' + 1 vbcrlf

                        ElseIf ((serialString.IndexOf(stringRecordTypeError, position) >= 0)) Then
                            position = position + stringRecordTypeError.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringNonAsciiDigits, position) >= 0)) Then
                            position = position + stringNonAsciiDigits.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringErrorDownloadBytes, position) >= 0)) Then
                            position = position + stringErrorDownloadBytes.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringFailedChecksume, position) >= 0)) Then
                            position = position + stringFailedChecksume.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringSFlashProgrammingError, position) >= 0)) Then
                            Dim theThread As New Threading.Thread(AddressOf auto_flash)
                            Dim thislock As New Object
                            SyncLock thislock
                                theThread.Abort()
                                currentBankId = 0
                                bankId = -1
                                serialString = ""
                                position = 0
                                cmdEraseFlash = 0
                                btn_autoFlash.Enabled = True
                                btn_abort.Enabled = False
                                ShrinkToolStripMenuItem_autoflash.Enabled = True
                                AbortToolStripMenuItem_abort.Enabled = False
                                errorsig = 1
                                Try
                                    With SerialPort1
                                        .DiscardOutBuffer()
                                    End With
                                Catch ex As Exception
                                    MsgBox("Unable to flush buffer")
                                End Try
                                ProgressBar1.Value = 0
                            End SyncLock
                        ElseIf ((serialString.IndexOf(stringErrorUARTToRAM, position) >= 0)) Then
                            Dim theThread As New Threading.Thread(AddressOf auto_flash)
                            Dim thislock As New Object
                            SyncLock thislock
                                theThread.Abort()
                                currentBankId = 0
                                bankId = -1
                                serialString = ""
                                position = 0
                                cmdEraseFlash = 0
                                btn_autoFlash.Enabled = True
                                btn_abort.Enabled = False
                                ShrinkToolStripMenuItem_autoflash.Enabled = True
                                AbortToolStripMenuItem_abort.Enabled = False
                                errorsig = 1
                                Try
                                    With SerialPort1
                                        .DiscardOutBuffer()
                                    End With
                                Catch ex As Exception
                                    MsgBox("Unable to flush buffer")
                                End Try
                                ProgressBar1.Value = 0
                            End SyncLock
                        Else
                            'position = SerialPort1.BytesToRead
                            Debug.Print("Else: " & serialString)
                        End If ' serial string parser
                        '''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''CRAM Code load'''''''''''''''''''''''''
                    ElseIf cmdEraseFlash = 2 Then
                        serialString = serialString & temp_char
                        If (serialString.IndexOf(stringGreenvity, position) >= 0) And (serialString.IndexOf(stringBootloader, position) >= 0) Then
                            position = position + stringGreenvity.Length + stringBootloader.Length
                            serialString = ""
                            position = 0
                            bankId = -1
                            currentBankId = 0
                            ProgressBar1.Value = 0
                            j = 0
                            Try
                                With SerialPort1
                                    .Write("U")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try
                        ElseIf (serialString.IndexOf(stringProgramFlash, position) >= 0) Then
                            position = position + stringProgramFlash.Length + 1 ' + 1 for vbcrlf
                            serialString = ""
                            position = 0
                            Try
                                With SerialPort1
                                    .Write("Y")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try
                        ElseIf (serialString.IndexOf(stringEraseFlash, position) >= 0) Then
                            position = position + stringEraseFlash.Length + 1 ' + 1 for vbcrlf
                            serialString = ""
                            position = 0
                            Try
                                With SerialPort1
                                    .Write("Y")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try

                        ElseIf (serialString.IndexOf(stringBankCramId0, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId0.Length + stringWaitingForIntelHexFile.Length + 3 '\n\n \n
                            serialString = ""
                            position = 0
                            ' Thread.Sleep(100)
                            bankId = 0
                            Debug.Print("waiting bank id " & bankId)
                            'ProgressBar1.ForeColor = Color.DarkRed
                            'SendMessage(ProgressBar1.Handle, 1040, 1, 0)
                            ProgressBar1.Value = 0

                        ElseIf (serialString.IndexOf(stringBankCramId1, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId1.Length + stringWaitingForIntelHexFile.Length
                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 1
                            Debug.Print("waiting bank id " & bankId)
                            ProgressBar1.Value = 10

                        ElseIf (serialString.IndexOf(stringBankCramId2, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId2.Length + stringWaitingForIntelHexFile.Length
                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 2
                            Debug.Print("waiting bank id " & bankId)
                            'MsgBox("isr")
                            ProgressBar1.Value = 20

                        ElseIf (serialString.IndexOf(stringBankCramId3, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId3.Length + stringWaitingForIntelHexFile.Length
                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 3
                            Debug.Print("waiting bank id " & bankId)
                            ProgressBar1.Value = 30

                        ElseIf (serialString.IndexOf(stringBankCramId4, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId4.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 4
                            Debug.Print("waiting bank id " & bankId)
                            ProgressBar1.Value = 40

                        ElseIf (serialString.IndexOf(stringBankCramId5, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId5.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 5
                            Debug.Print("waiting bank id " & bankId)
                            ProgressBar1.Value = 50

                        ElseIf (serialString.IndexOf(stringBankCramId6, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId6.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 6
                            Debug.Print("waiting bank id " & bankId)
                            ProgressBar1.Value = 60

                        ElseIf (serialString.IndexOf(stringBankCramId7, position) >= 0) And (serialString.IndexOf(stringWaitingForIntelHexFile, position) >= 0) Then
                            position = position + stringBankId7.Length + stringWaitingForIntelHexFile.Length

                            serialString = ""
                            position = 0
                            'Thread.Sleep(100)
                            bankId = 7
                            Debug.Print("waiting bank id " & bankId)
                            ProgressBar1.Value = 70

                            'ElseIf (serialString.IndexOf(stringRunningFw, position) >= 0) Then
                            '    position = position + stringBankId7.Length + stringWaitingForIntelHexFile.Length

                            '    serialString = ""
                            '    position = 0
                            '    MsgBox("Run FW detected")
                            '    Try
                            '        With SerialPort1
                            '            .BaudRate = combobx_baudRate2.Text
                            '        End With
                            '    Catch ex As Exception

                            '    End Try
                        ElseIf (serialString.IndexOf(stringRequestResetChar, position) >= 0) Then
                            position = 0
                            serialString = ""
                            Try
                                With SerialPort1
                                    .Write("s")
                                    Debug.Print("Reset send s")
                                End With
                            Catch ex As Exception
                                MessageBox.Show("Serial Port not available/disconnected")
                            End Try

                        ElseIf ((serialString.IndexOf(stringWritingToSFlash, position) >= 0)) Then
                            position = position + stringWritingToSFlash.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringSummery, position) >= 0)) Then
                            position = position + stringSummery.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringSuccessDownloadBytes, position) >= 0)) Then
                            position = position + stringSuccessDownloadBytes.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringFoundLines, position) >= 0)) Then
                            position = position + stringFoundLines.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringFalseLines, position) >= 0)) Then
                            position = position + stringFalseLines.Length + 1 ' + 1 vbcrlf

                        ElseIf ((serialString.IndexOf(stringRecordTypeError, position) >= 0)) Then
                            position = position + stringRecordTypeError.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringNonAsciiDigits, position) >= 0)) Then
                            position = position + stringNonAsciiDigits.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringErrorDownloadBytes, position) >= 0)) Then
                            position = position + stringErrorDownloadBytes.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringFailedChecksume, position) >= 0)) Then
                            position = position + stringFailedChecksume.Length + 1 ' + 1 vbcrlf

                            serialString = ""
                            position = 0

                        ElseIf ((serialString.IndexOf(stringSFlashProgrammingError, position) >= 0)) Then
                            Dim theThread As New Threading.Thread(AddressOf auto_flash)
                            Dim thislock As New Object
                            SyncLock thislock
                                theThread.Abort()
                                currentBankId = 0
                                bankId = -1
                                serialString = ""
                                position = 0
                                cmdEraseFlash = 0
                                btn_autoFlash.Enabled = True
                                btn_abort.Enabled = False
                                ShrinkToolStripMenuItem_autoflash.Enabled = True
                                AbortToolStripMenuItem_abort.Enabled = False
                                errorsig = 1
                                Try
                                    With SerialPort1
                                        .DiscardOutBuffer()
                                    End With
                                Catch ex As Exception
                                    MsgBox("Unable to flush buffer")
                                End Try
                                ProgressBar1.Value = 0
                            End SyncLock
                        ElseIf ((serialString.IndexOf(stringErrorUARTToRAM, position) >= 0)) Then
                            Dim theThread As New Threading.Thread(AddressOf auto_flash)
                            Dim thislock As New Object
                            SyncLock thislock
                                theThread.Abort()
                                currentBankId = 0
                                bankId = -1
                                serialString = ""
                                position = 0
                                cmdEraseFlash = 0
                                btn_autoFlash.Enabled = True
                                btn_abort.Enabled = False
                                ShrinkToolStripMenuItem_autoflash.Enabled = True
                                AbortToolStripMenuItem_abort.Enabled = False
                                errorsig = 1
                                Try
                                    With SerialPort1
                                        .DiscardOutBuffer()
                                    End With
                                Catch ex As Exception
                                    MsgBox("Unable to flush buffer")
                                End Try
                                ProgressBar1.Value = 0
                            End SyncLock
                        Else
                            'position = SerialPort1.BytesToRead
                            Debug.Print("Else: " & serialString)
                        End If

                    End If

                    ' temp_char = System.Text.Encoding.ASCII.GetString(bytebuffer, 0, bytecount)
                    ' txtbx_console.AppendText(temp_char)
                    'If temp_char.StartsWith(vbCrLf & "*") Then
                    '    MsgBox("vbcrlf Working")
                    'End If
                    'txtbx_console.SelectionStart = txtbx_console.Text.Length
                    'txtbx_console.ScrollToCaret()
                Catch ex As Exception
                    MsgBox("Error:" & ex.ToString, MsgBoxStyle.Critical)
                End Try
            Loop
        End If
    End Sub

    ' Private Sub txtbx_console_MouseWheel(sender As Object, e As MouseEventArgs) Handles txtbx_console.MouseWheel
    '    Dim si As New SCROLLINFO

    '   With si
    '      .cbSize = Len(si)

    '     .fMask = ScrollInfoMask.SIF_ALL
    'End With

    'Dim lRet As Integer = GetScrollInfo(txtbx_console.Handle, SBS_VERT, si)
    'If lRet <> 0 Then
    '   MsgBox("max " & si.nMax & " min " & si.nMin & "  page " & si.nPage & " track " & si.nTrackPos & " pos " & si.nPos)
    'End If

    'TextBox1.AppendText((GetScrollPos(txtbx_console.Handle, SBS_VERT)).ToString & vbCrLf)
    'End Sub
    Private Sub txtbx_console_TextChanged(sender As Object, e As System.EventArgs) Handles txtbx_console.TextChanged
        ' If (txtbx_console. = 0) Then
        'End If
        '   Dim si As New SCROLLINFO

        '  With si
        '     .cbSize = Len(si)

        '    .fMask = ScrollInfoMask.SIF_ALL
        'End With
        'Dim lRet As Integer = GetScrollInfo(txtbx_console.Handle, SBS_VERT, si)
        'TextBox2.AppendText((GetScrollPos(txtbx_console.Handle, SBS_VERT)).ToString & vbCrLf)
        'If lRet <> 0 Then
        '   If si.nPos = si.nPage Then
        With Me.txtbx_console
            .SelectionStart = Len(.Text)
            .ScrollToCaret()
            '          MsgBox(si.nPos & " " & si.nPage)
        End With
        ' Else

        'End If
        'End If
    End Sub

    Private Sub btn_autoFlash_Click(sender As System.Object, e As System.EventArgs) Handles btn_autoFlash.Click
        flash_state_cr = LOAD_BOOT
        Try
            With SerialPort1
                .BaudRate = combobx_baudRate1.Text
            End With
        Catch ex As Exception

        End Try
        autoflash_sub()
    End Sub
    Private Sub ShrinkToolStripMenuItem_autoflash_Click(sender As System.Object, e As System.EventArgs) Handles ShrinkToolStripMenuItem_autoflash.Click
        flash_state_cr = LOAD_BOOT
        Try
            With SerialPort1
                .BaudRate = combobx_baudRate1.Text
            End With
        Catch ex As Exception

        End Try
        autoflash_sub()
    End Sub
    Private Sub autoflash_sub()
        Dim procid
        procid = Process.GetCurrentProcess.Id
        save_path()
        If SerialPort1.IsOpen Then
            Me.Text = PRODUCT_TITLE & Application.ProductVersion & " - PID: " & procid.ToString & " - Connected to " & SerialPort1.PortName.ToString & " - File: " & txtbx_fileName.Text
        Else
            Me.Text = PRODUCT_TITLE & Application.ProductVersion & " - PID: " & procid.ToString & " - Comm " & "Disconnected" & " - File: " & txtbx_fileName.Text
        End If
        Dim theThread As New Threading.Thread(AddressOf auto_flash)
        btn_autoFlash.Enabled = False
        btn_abort.Enabled = True
        ShrinkToolStripMenuItem_autoflash.Enabled = False
        AbortToolStripMenuItem_abort.Enabled = True
        bankId = -1
        currentBankId = 0
        position = 0
        serialString = ""

        If flash_state_cr = LOAD_BOOT Then
            'chk_bx_load_to_cram.Checked = True
            Debug.Print("autoflash_sub: flash_state_cr = LOAD_BOOT")

            radiobtn_allBank.Checked = False
            radiobtn_customBank.Checked = True
            chkbx_b0.Checked = True
            chkbx_b1.Checked = False
            chkbx_b2.Checked = False
            chkbx_b3.Checked = False
            chkbx_b4.Checked = False
            chkbx_b5.Checked = False
            chkbx_b6.Checked = False
            chkbx_b7.Checked = False

            chkbx_b1.Enabled = True
            chkbx_b2.Enabled = True
            chkbx_b3.Enabled = True
            chkbx_b4.Enabled = True
            chkbx_b5.Enabled = True
            chkbx_b6.Enabled = True
            chkbx_b7.Enabled = True

        ElseIf flash_state_cr = LOAD_FW Then
            ' chk_bx_load_to_cram.Checked = False
            Debug.Print("autoflash_sub: flash_state_cr = LOAD_FW")

            radiobtn_allBank.Checked = True
            radiobtn_customBank.Checked = False
            chkbx_b0.Enabled = False
            chkbx_b1.Enabled = False
            chkbx_b2.Enabled = False
            chkbx_b3.Enabled = False
            chkbx_b4.Enabled = False
            chkbx_b5.Enabled = False
            chkbx_b6.Enabled = False
            chkbx_b7.Enabled = False

            chkbx_b0.Checked = True
            chkbx_b1.Checked = True
            chkbx_b2.Checked = True
            chkbx_b3.Checked = True
            chkbx_b4.Checked = True
            chkbx_b5.Checked = True
            chkbx_b6.Checked = True
            chkbx_b7.Checked = True

        End If

        If chk_bx_load_to_cram.Checked = True Then
            cmdEraseFlash = 2
        Else
            cmdEraseFlash = 1
        End If

        ProgressBar1.Value = 0
        errorsig = 0
        'theThread.Priority = ThreadPriority.Normal
        If chkbx_rts.Checked = True Then
            Try
                With SerialPort1
                    .DtrEnable = True
                    Thread.Sleep(100)
                    .DtrEnable = False
                End With
            Catch ex As Exception

            End Try
        End If
        Debug.Print("autoflash_sub: autoflash thread started")
        theThread.Start()
        ' auto_flash()
    End Sub
    Private Sub btn_abort_Click(sender As System.Object, e As System.EventArgs) Handles btn_abort.Click
        flash_abort_sub()
    End Sub
    Private Sub AbortToolStripMenuItem_abort_Click(sender As System.Object, e As System.EventArgs) Handles AbortToolStripMenuItem_abort.Click
        flash_abort_sub()
    End Sub
    Private Sub flash_abort_sub()
        Dim theThread As New Threading.Thread(AddressOf auto_flash)
        Dim thislock As New Object
        SyncLock thislock
            theThread.Abort()

            Try
                With SerialPort1
                    .DiscardOutBuffer()
                End With

            Catch ex As Exception

            End Try

            btn_autoFlash.Enabled = True
            btn_abort.Enabled = False
            ShrinkToolStripMenuItem_autoflash.Enabled = True
            AbortToolStripMenuItem_abort.Enabled = False
            bankId = -1
            currentBankId = 0
            cmdEraseFlash = 0
            position = 0
            serialString = ""
            ProgressBar1.Value = 0
            errorsig = 1
            j = 0 ' clear for loop j counter
        End SyncLock
    End Sub
    Sub auto_flash()
        Dim filepath As String = ""
        Dim fs As FileStream
        Dim sr As StreamReader
        Dim procid
        Dim flag As Integer = 0
        Dim pos As Integer = 0
        Dim extension As String = ""
        Dim filename As String = ""
        Dim filenamepath As String
        procid = Process.GetCurrentProcess.Id

        Control.CheckForIllegalCrossThreadCalls = False


        If txtbx_filePath.TextLength > 0 And txtbx_fileName.TextLength > 0 Then

            If flash_state_cr = LOAD_BOOT Then
                'chk_bx_load_to_cram.Checked = True
                filenamepath = Path.GetFullPath(Application.ExecutablePath)
                pos = filenamepath.LastIndexOf("\")
                filepath = filenamepath.Remove(pos, filenamepath.Length - pos)

                filename = stringBootLoaderFileName
            ElseIf flash_state_cr = LOAD_FW Then
                'chk_bx_load_to_cram.Checked = False
                filename = txtbx_fileName.Text
            End If

            ' filename = txtbx_fileName.Text
            pos = filename.LastIndexOf(".")
            extension = UCase(Strings.Right(filename, (filename.Length - pos - 1)))
            ' UCASE reduces double comparision of HEX and hex extension

            If (extension = "HEX") Then
                hexFileExtensionHex = 1

                If flash_state_cr = LOAD_BOOT Then
                    'chk_bx_load_to_cram.Checked = True
                    filenamepath = Path.GetFullPath(Application.ExecutablePath)
                    pos = filenamepath.LastIndexOf("\")
                    filepath = filenamepath.Remove(pos, filenamepath.Length - pos)

                    filename = stringBootLoaderFileName
                    filepath = filepath & "\" & filename
                ElseIf flash_state_cr = LOAD_FW Then
                    'chk_bx_load_to_cram.Checked = False
                    filename = txtbx_fileName.Text
                    filepath = txtbx_filePath.Text & "\" & txtbx_fileName.Text
                End If

                Try
                    fs = New FileStream(filepath, FileMode.Open)
                    sr = New StreamReader(fs)
                    hex_file = sr.ReadToEnd
                    sr.Close()
                    fs.Close()
                    hex_editor_sub(hex_file)
                Catch ex As Exception
                    MsgBox("Error: " & ex.ToString, MsgBoxStyle.Critical)
                    btn_autoFlash.Enabled = True
                    btn_abort.Enabled = False
                    ShrinkToolStripMenuItem_autoflash.Enabled = True
                    AbortToolStripMenuItem_abort.Enabled = False
                    bankId = -1
                    currentBankId = 0
                    cmdEraseFlash = 0
                    position = 0
                    serialString = ""
                    j = 0
                    Exit Sub
                End Try
            Else
                hexFileExtensionHex = 0

            End If

            For j = 0 To 7
                While (bankId <> currentBankId)
                    Thread.Sleep(500)
                    If errorsig = 1 Then
                        errorsig = 0
                        'MsgBox("I am strucked")
                        Exit Sub
                    End If
                End While
                currentBankId = currentBankId + 1
                If (radiobtn_customBank.Checked = True) Then
                    'MsgBox(bankId.ToString)
                    Select Case bankId

                        Case 0
                            If (chkbx_b0.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 1
                            If (chkbx_b1.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 2
                            If (chkbx_b2.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 3
                            If (chkbx_b3.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 4
                            If (chkbx_b4.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 5
                            If (chkbx_b5.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 6
                            If (chkbx_b6.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                        Case 7
                            If (chkbx_b7.Checked = True) Then
                                flag = 0
                            Else
                                flag = 1
                            End If
                    End Select
                Else
                    flag = 0
                End If
                'Dim result As MsgBoxResult = MessageBox.Show("Continue File " & txtbx_fileName.Text & ".H0" & j.ToString & "?", "GV701x Flash... PID = " & procid.ToString, MessageBoxButtons.YesNoCancel)
                If (flag = 1) Then
                    SerialPort1.Write(Chr(27)) ' sends escape key over serial port
                    ' Thread.Sleep(300)
                    'SerialPort1.Write(stringEofRecord)

                    Debug.Print("stringEofRecord")
                Else

                    Try
                        If (hexFileExtensionHex = 0) Then
                            'filepath = txtbx_filePath.Text & "\" & txtbx_fileName.Text & ".H0" & j.ToString

                            If flash_state_cr = LOAD_BOOT Then
                                'chk_bx_load_to_cram.Checked = True
                                filenamepath = Path.GetFullPath(Application.ExecutablePath)
                                pos = filenamepath.LastIndexOf("\")
                                filepath = filenamepath.Remove(pos, filenamepath.Length - pos)
                                Debug.Print(filepath)
                                filename = stringBootLoaderFileName
                                lbl_filename.Text = stringBootLoaderFileName '& ".H0" & j.ToString
                                filepath = filepath & "\" & filename
                            ElseIf flash_state_cr = LOAD_FW Then
                                'chk_bx_load_to_cram.Checked = False
                                filename = txtbx_fileName.Text
                                lbl_filename.Text = txtbx_fileName.Text & ".H0" & j.ToString
                                filepath = txtbx_filePath.Text & "\" & txtbx_fileName.Text & ".H0" & j.ToString
                            End If

                            fs = New FileStream(filepath, FileMode.Open)
                            sr = New StreamReader(fs)
                            hex_file = sr.ReadToEnd
                            sr.Close()
                            fs.Close()
                        Else
                            If j = -1 Then
                                hex_file = bank_file_common
                                lbl_filename.Text = filename & ": Bank: " & "Common"
                            Else
                                hex_file = bank_file(j)
                                lbl_filename.Text = filename & ": Bank: " & j.ToString
                            End If

                        End If
                        'For i = 0 To (hex_file.Length - 1)
                        Try
                            Debug.Print("Bank id write" & j.ToString)
                            SerialPort1.Write(hex_file, 0, hex_file.Length)

                        Catch ex As Exception
                            MsgBox("Error: " & ex.ToString, MsgBoxStyle.Critical)

                            btn_autoFlash.Enabled = True
                            btn_abort.Enabled = False
                            ShrinkToolStripMenuItem_autoflash.Enabled = True
                            AbortToolStripMenuItem_abort.Enabled = False
                            bankId = -1
                            currentBankId = 0
                            cmdEraseFlash = 0
                            position = 0
                            serialString = ""
                            j = 0
                            Exit Sub
                        End Try
                        'Next
                    Catch ex As Exception ' file stream
                        MsgBox("Error: " & ex.ToString, MsgBoxStyle.Critical)
                        btn_autoFlash.Enabled = True
                        btn_abort.Enabled = False
                        ShrinkToolStripMenuItem_autoflash.Enabled = True
                        AbortToolStripMenuItem_abort.Enabled = False
                        bankId = -1
                        currentBankId = 0
                        cmdEraseFlash = 0
                        position = 0
                        serialString = ""
                        j = 0
                        Exit Sub
                    End Try
                End If

            Next
        End If

        btn_autoFlash.Enabled = True
        btn_abort.Enabled = False
        ShrinkToolStripMenuItem_autoflash.Enabled = True
        AbortToolStripMenuItem_abort.Enabled = False
        bankId = -1
        currentBankId = 0
        cmdEraseFlash = 0

        If flash_state_cr = LOAD_FW Then
            flash_state_cr = LOAD_IDLE
            Debug.Print("autoflash thread: LOAD_IDLE")
            ProgressBar1.Value = 80
            'SerialPort1.BaudRate = 28800
        Else
            flash_state_cr = LOAD_FW
            cmdEraseFlash = 1
            Debug.Print("autoflash thread: LOAD_FW")
            ProgressBar1.Value = 0
        End If

        position = 0
        serialString = ""

    End Sub

    Sub load_path()
        Dim pos As Integer
        Dim filenamepath As String
        Dim filepath As String

        filenamepath = Path.GetFullPath(Application.ExecutablePath)

        pos = filenamepath.LastIndexOf("\")
        filepath = filenamepath.Remove(pos, filenamepath.Length - pos)

        Dim setting_reader As XmlTextReader = New XmlTextReader(filepath & "\" & "settings.xml")
        Dim field As String = " " ' initialize to avoid null reference
        Try
            Do While (setting_reader.Read())
                Select Case setting_reader.NodeType
                    Case XmlNodeType.Element
                        'txtbx_console.AppendText("Element  <" + setting_reader.Name)
                        field = setting_reader.Name

                        'If setting_reader.HasAttributes Then
                        '    While setting_reader.MoveToNextAttribute
                        '        txtbx_console.AppendText("Attribute " & setting_reader.Name & "    " & setting_reader.Value)
                        '    End While
                        'End If
                        'txtbx_console.AppendText(">" & vbCrLf)
                    Case XmlNodeType.Text
                        'txtbx_console.AppendText("Text = " & setting_reader.Value & vbCrLf)
                        If field = "filepath" Then
                            txtbx_filePath.Text = setting_reader.Value
                        ElseIf field = "filename" Then
                            txtbx_fileName.Text = setting_reader.Value
                        End If
                    Case XmlNodeType.EndElement
                        'txtbx_console.AppendText("</" & setting_reader.Name & ">" & vbCrLf)
                End Select
            Loop
        Catch ex As Exception
            MsgBox("Manually delete settings.xml", MsgBoxStyle.Critical)
            ' if file is corrupted then overwrite settings.xml file
        End Try
    End Sub
    Sub save_path()
        'Dim strPath As String = System.IO.Path.GetDirectoryName( _
        '		System.Reflection.Assembly.GetExecutingAssembly().CodeBase) ' This gives extra details like file:\ at start of string
        'msgbox(strPath)	
        Dim pos As Integer
        Dim filenamepath As String
        Dim filepath As String

        filenamepath = Path.GetFullPath(Application.ExecutablePath)

        pos = filenamepath.LastIndexOf("\")
        filepath = filenamepath.Remove(pos, filenamepath.Length - pos)
        If IO.File.Exists(filepath & "\" & "settings.xml") = True Then
            Dim setting_xml As New XmlDocument
            Try
                setting_xml.Load(filepath & "\" & "settings.xml")
            Catch ex As Exception
                MsgBox("Manually delete settings.xml", MsgBoxStyle.Critical)
                Exit Sub
            End Try
            Dim settingsnode As XmlNode = setting_xml.SelectSingleNode("/filedetails")
            If settingsnode IsNot Nothing Then
                settingsnode.ChildNodes(0).InnerText = txtbx_filePath.Text
                settingsnode.ChildNodes(1).InnerText = txtbx_fileName.Text
                Try
                    setting_xml.Save(filepath & "\" & "settings.xml")
                Catch ex As Exception

                End Try
            End If
        Else
            Dim settings_xml As New XmlWriterSettings
            settings_xml.Indent = True
            Dim xmlwrt As XmlWriter = XmlWriter.Create(filepath & "\" & "settings.xml", settings_xml)
            With xmlwrt
                .WriteStartDocument()
                .WriteStartElement("filedetails")
                .WriteStartElement("filepath")
                If (txtbx_filePath.TextLength <> 0) Then
                    .WriteString(txtbx_filePath.Text)
                Else
                    .WriteString("c:\")
                    txtbx_filePath.Text = "c:\"
                End If
                .WriteEndElement()

                .WriteStartElement("filename")
                If (txtbx_fileName.TextLength <> 0) Then
                    .WriteString(txtbx_fileName.Text)
                Else
                    .WriteString("gchpgp")
                    txtbx_fileName.Text = "gchpgp"
                End If
                .WriteEndElement()
                .WriteEndElement()
                .WriteEndDocument()
                .Close()
            End With
        End If
    End Sub
    Sub sub_shrink()
        ' grbx_flashOption.Location = New Point(12, 31)
        '  grbx_flashOption.Location = New Point(12, 31)
        Dim screenWidth As Integer = Screen.PrimaryScreen.Bounds.Width
        Dim screenHeight As Integer = Screen.PrimaryScreen.Bounds.Height

        grbx_flashOption.Visible = False
        grbx_hexfilepath.Visible = False
        grbx_serial_settings.Visible = False
        'MsgBox(screenWidth & " " & screenHeight)
        If (screenWidth < 1920) Then
            tab_ctrl_flashmode.Size = New Size(650, 380)
            tabPage1.Size = New Size(640, 352)
            Me.Size = New Size(680, 450)
            tab_ctrl_flashmode.Location = New Point(12, 31)
            grbx_console.Location = New Point(13, 10)
        Else
            tab_ctrl_flashmode.Size = New Size(650 * 1.5, 380 * 1.5)
            tabPage1.Size = New Size(640 * 1.5, 352 * 1.5)
            Me.Size = New Size(680 * 1.3, 450 * 1.25)
            tab_ctrl_flashmode.Location = New Point(12 * 1.5, 31 * 1.5)
            grbx_console.Location = New Point(13 * 1.5, 10 * 1.5)
        End If
    End Sub
    
    Sub sub_expand()
        Me.Size = form_size 'New Size(860, 612)

        tab_ctrl_flashmode.Size = flash_mode_size
        tabPage1.Size = tabPage1_Size
        grbx_flashOption.Location = grbx_flashOption_Location
        tab_ctrl_flashmode.Location = tab_ctrl_flashmode_Location

        grbx_serial_settings.Visible = True
        grbx_console.Location = grbx_console_Location
        grbx_hexfilepath.Visible = True
        grbx_flashOption.Visible = True
        'tab_ctrl_flashmode.Location = New Point(12, 100)
        'tab_ctrl_flashmode.Size = New Size(807, 485)
        ' tabPage1.Size = New Size(799, 459)
        ' grbx_flashOption.Location = New Point(881, 12)
        'grbx_console.Location = New Point(13, 100)
    End Sub
    
    Private Sub ShrinkToolStripMenuItem1_Click(sender As System.Object, e As System.EventArgs) Handles ShrinkToolStripMenuItem1.Click
        sub_shrink()

    End Sub
    
    Private Sub ExpandToolStripMenuItem1_Click(sender As System.Object, e As System.EventArgs) Handles ExpandToolStripMenuItem1.Click
        sub_expand()
    End Sub


    Private Sub btn_701xReset_Click(sender As System.Object, e As System.EventArgs) Handles btn_701xReset.Click
        Try
            With SerialPort1
                .DtrEnable = True
                Thread.Sleep(300)
                .DtrEnable = False
            End With
        Catch ex As Exception

        End Try
    End Sub

    Private Sub radiobtn_allBank_CheckedChanged(sender As System.Object, e As System.EventArgs) Handles radiobtn_allBank.CheckedChanged
        chkbx_b0.Enabled = False
        chkbx_b1.Enabled = False
        chkbx_b2.Enabled = False
        chkbx_b3.Enabled = False
        chkbx_b4.Enabled = False
        chkbx_b5.Enabled = False
        chkbx_b6.Enabled = False
        chkbx_b7.Enabled = False
    End Sub

    Private Sub radiobtn_customBank_CheckedChanged(sender As System.Object, e As System.EventArgs) Handles radiobtn_customBank.CheckedChanged
        chkbx_b0.Enabled = True
        chkbx_b1.Enabled = True
        chkbx_b2.Enabled = True
        chkbx_b3.Enabled = True
        chkbx_b4.Enabled = True
        chkbx_b5.Enabled = True
        chkbx_b6.Enabled = True
        chkbx_b7.Enabled = True
    End Sub

    Private Sub ConsoleClearToolStripMenuItem_Click(sender As System.Object, e As System.EventArgs) Handles ConsoleClearToolStripMenuItem.Click
        txtbx_console.Clear()
    End Sub

    Private Sub AboutToolStripMenuItem_Click(sender As Object, e As EventArgs)
        'SplashScreen1.Show()
    End Sub

    Private Sub btn_portSearch_Click(sender As System.Object, e As System.EventArgs) Handles btn_portSearch.Click
        combobx_portNo.Items.Clear()

        For Each sp As String In My.Computer.Ports.SerialPortNames
            combobx_portNo.Items.Add(sp)
        Next

        If combobx_portNo.Items.Count = 0 Then
            MsgBox("Serial Port not found", MsgBoxStyle.Information)
        Else
            combobx_portNo.SelectedIndex = 0
        End If
    End Sub

    Private Sub btn_pathBrowse_Click(sender As System.Object, e As System.EventArgs) Handles btn_pathBrowse.Click
        filepath_sub()
    End Sub

    Private Sub ExpandToolStripMenuItem_file_path_Click(sender As System.Object, e As System.EventArgs) Handles ExpandToolStripMenuItem_file_path.Click
        filepath_sub()
    End Sub

    Private Sub Form1_Resize(sender As Object, e As EventArgs) Handles Me.Resize

    End Sub

    Private Sub ExitToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles ExitToolStripMenuItem.Click
        Me.Close()
    End Sub

    Private Sub txtbx_mac_addr_TextChanged(sender As Object, e As EventArgs) Handles txtbx_mac_addr.TextChanged
        ' If Not "1234567890ABCDEF".Contains(Char.ToUpper(e.KeyChar)) AndAlso e.KeyChar <> vbBack Then
        'e.Handled = True
        'End If
        'For Each i In txtbx_mac_addr.ToString
        '    If Not (System.Uri.IsHexDigit(i) Or i <> ":") Then
        '        MsgBox("It has no valid Hex character")
        '        Exit For
        '    End If
        'Next
    End Sub

    Private Sub txtbx_mac_addr_KeyPress(sender As Object, e As KeyPressEventArgs) Handles txtbx_mac_addr.KeyPress
        'If Not "1234567890ABCDEF".Contains(Char.ToUpper(e.KeyChar)) AndAlso e.KeyChar <> vbBack Then
        'e.Handled = True
        'End If

        e.Handled = Not (System.Uri.IsHexDigit(e.KeyChar) Or (e.KeyChar = ":") Or (e.KeyChar = vbBack))

    End Sub



    Private Sub btn_commit_Click(sender As Object, e As EventArgs) Handles btn_commit.Click
        Dim commit As String = "p commit" & vbCrLf
        btn_commit.Enabled = False
        If SerialPort1.IsOpen Then
            Try
                With SerialPort1
                    For Each i In commit
                        .Write(i)
                        Thread.Sleep(100)
                    Next
                    '.Write(commit, 0, commit.Length)
                    ' Thread.Sleep(1000)
                End With

            Catch ex As Exception
                MsgBox(e.ToString)
            End Try
        Else
            MsgBox("Serial Port not open or available")
        End If
        btn_commit.Enabled = True
    End Sub

    Private Sub btn_send_mac_Click(sender As Object, e As EventArgs) Handles btn_send_mac.Click
        Dim error_char As Boolean = True
        Dim char_counter As Integer = 0

        btn_send_mac.Enabled = False

        If txtbx_mac_addr.TextLength = 17 Then
            For Each i In txtbx_mac_addr.Text
                Select Case char_counter
                    Case 2, 5, 8, 11, 14
                        char_counter += 1
                        If i <> ":" Then
                            MsgBox("It has no valid Hex character : " & i)
                            error_char = False
                            Exit For
                        End If
                    Case Else
                        char_counter += 1
                        If Not (System.Uri.IsHexDigit(i)) Then
                            MsgBox("It has no valid Hex character : " & i)
                            error_char = False
                            Exit For
                        End If
                End Select

            Next

            If error_char = True Then
                If SerialPort1.IsOpen Then
                    Try
                        With SerialPort1
                            For Each i In "p setmac" & vbCr
                                .Write(i)
                                Thread.Sleep(100)
                            Next
                            '.Write(commit, 0, commit.Length)
                            ' Thread.Sleep(1000)
                        End With

                    Catch ex As Exception
                        MsgBox(e.ToString)
                    End Try
                Else
                    MsgBox("Serial Port not open or available")
                End If

                Thread.Sleep(1000)

                If SerialPort1.IsOpen Then
                    Try
                        With SerialPort1
                            For Each i In txtbx_mac_addr.Text & vbCr
                                .Write(i)
                                Thread.Sleep(100)
                            Next

                        End With

                    Catch ex As Exception
                        MsgBox(e.ToString)
                    End Try
                Else
                    MsgBox("Serial Port not open or available")
                End If
            End If
        Else
            MsgBox("Check MAC Address Contents")
        End If
        btn_send_mac.Enabled = True
    End Sub

    Private Sub txtbx_defKey_KeyPress(sender As Object, e As KeyPressEventArgs) Handles txtbx_defKey.KeyPress
        e.Handled = Not (System.Uri.IsHexDigit(e.KeyChar) Or (e.KeyChar = vbBack))
    End Sub

    Private Sub btn_send_key_Click(sender As Object, e As EventArgs) Handles btn_send_key.Click
        Dim error_char As Boolean = True
        Dim char_counter As Integer = 0

        btn_send_key.Enabled = False

        If txtbx_defKey.TextLength = 32 Then
            For Each i In txtbx_defKey.Text

                If Not (System.Uri.IsHexDigit(i)) Then
                    MsgBox("It has no valid Hex character : " & i)
                    error_char = False
                    Exit For
                End If
            Next

            If error_char = True Then
                If SerialPort1.IsOpen Then
                    Try
                        With SerialPort1
                            For Each i In "p dkey" & vbCr
                                .Write(i)
                                Thread.Sleep(100)
                            Next

                        End With

                    Catch ex As Exception
                        MsgBox(e.ToString)
                    End Try
                Else
                    MsgBox("Serial Port not open or available")
                End If

                Thread.Sleep(1000)

                If SerialPort1.IsOpen Then
                    Try
                        With SerialPort1
                            For Each i In txtbx_defKey.Text & vbCr
                                .Write(i)
                                Thread.Sleep(100)
                            Next

                        End With

                    Catch ex As Exception
                        MsgBox(e.ToString)
                    End Try
                Else
                    MsgBox("Serial Port not open or available")
                End If

            End If
        Else
            MsgBox("Check Key Contents")
        End If
        btn_send_key.Enabled = True
    End Sub

    Private Sub txtbx_defKey_TextChanged(sender As Object, e As EventArgs) Handles txtbx_defKey.TextChanged
        lbl_key_length.Text = txtbx_defKey.TextLength
    End Sub

    Private Sub btn_cin_clear_Click(sender As Object, e As EventArgs) Handles btn_cin_clear.Click
        btn_cin_clear.Enabled = False

        If SerialPort1.IsOpen Then
            Try
                With SerialPort1
                    For Each i In txtbx_cin_clear.Text & vbCr
                        .Write(i)
                        Thread.Sleep(100)
                    Next

                End With

            Catch ex As Exception
                MsgBox(e.ToString)
            End Try
        Else
            MsgBox("Serial Port not open or available")
        End If

        btn_cin_clear.Enabled = True

    End Sub

    Private Sub btn_device_type_Click(sender As Object, e As EventArgs) Handles btn_device_type.Click
        btn_device_type.Enabled = False

        If SerialPort1.IsOpen Then
            Try
                With SerialPort1
                    For Each i In txtbx_device_type.Text & vbCr
                        .Write(i)
                        Thread.Sleep(100)
                    Next

                End With

            Catch ex As Exception
                MsgBox(e.ToString)
            End Try
        Else
            MsgBox("Serial Port not open or available")
        End If

        btn_device_type.Enabled = True
    End Sub

    Private Sub txtbx_console_KeyDown(sender As Object, e As KeyEventArgs) Handles txtbx_console.KeyDown

        nonNumberEntered = False

        ' handles ctrl + key -  for standard windows supported commands  
        If e.Modifiers = Keys.Control And e.KeyCode = Keys.A Then
            nonNumberEntered = True
            txtbx_console.SelectAll()

            e.Handled = True
        ElseIf e.Modifiers = Keys.Control And e.KeyCode = Keys.V Then
            nonNumberEntered = True
            If My.Computer.Clipboard.ContainsText Then
                Dim Clipboard As String = My.Computer.Clipboard.GetText
                xmitString(Clipboard)

            End If
            e.Handled = True
        ElseIf e.Modifiers = Keys.Control And e.KeyCode = Keys.C Then
            nonNumberEntered = True
            If txtbx_console.SelectedText <> "" Then ' check if text is selected or not
                Clipboard.SetText(txtbx_console.SelectedText)

                e.Handled = True
            End If

        End If

        ' handles ctrl + alt + key - for tool control commands
        If e.Control And e.Alt And e.KeyCode = Keys.F Then
            nonNumberEntered = True
            autoflash_sub()
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.A Then
            nonNumberEntered = True
            flash_abort_sub()
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.C Then
            nonNumberEntered = True
            Clipboard.SetText(txtbx_console.Text)
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.P Then
            nonNumberEntered = True
            portconnect_sub()
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.O Then
            nonNumberEntered = True
            filepath_sub()
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.S Then
            nonNumberEntered = True
            sub_shrink()
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.E Then
            nonNumberEntered = True
            sub_expand()
            e.Handled = True
        ElseIf e.Control And e.Alt And e.KeyCode = Keys.Z Then
            nonNumberEntered = True
            txtbx_console.Clear()
            e.Handled = True
        End If

        ' handles ctrl + shift + key - for firmware control commands
        If e.Control And e.Shift And e.KeyCode = Keys.S Then
            nonNumberEntered = True
            xmitString("p stat")

            e.Handled = True
        ElseIf e.Control And e.Shift And e.KeyCode = Keys.U Then
            nonNumberEntered = True
            xmitString("p uartstat")

            e.Handled = True

        ElseIf e.Control And e.Shift And e.KeyCode = Keys.G Then
            nonNumberEntered = True
            xmitString("p gvmsg")

            e.Handled = True
        ElseIf e.Control And e.Shift And e.KeyCode = Keys.N Then
            nonNumberEntered = True
            xmitString("p nogvmsg")

            e.Handled = True
        End If
        ' e.Handled = True
    End Sub
    Sub xmitString(buf As String)
        If SerialPort1.IsOpen Then
            Try
                With SerialPort1
                    For Each chararacter In buf & vbCr
                        .Write(chararacter)
                        Thread.Sleep(50)
                    Next
                End With

            Catch ex As Exception
                MsgBox(ex.ToString)
            End Try

        Else
            MsgBox("Serial Port not open or available")
        End If
    End Sub
    Private Sub HelpToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles HelpToolStripMenuItem.Click

    End Sub

    Private Sub ShortcutKeysReferenceToolStripMenuItem_Click(sender As Object, e As EventArgs) Handles ShortcutKeysReferenceToolStripMenuItem.Click
        MessageBox.Show(
               "Copy Complete Console   - CTRL  +   ALT +   C" & vbCrLf &
               "Clear Console           - CTRL  +   ALT +   Z" & vbCrLf &
               "To Expand Tool Window   - CTRL  +   ALT +   E" & vbCrLf &
               "To Shrink Window        - CTRL  +   ALT +   S" & vbCrLf & vbCrLf &
               "To Connect & Disconnect Serial Port  - CTRL  +   ALT +   P" & vbCrLf &
               "To Open & Select File for Flashing  - CTRL  +   ALT +   O" & vbCrLf &
               "To Start Flashing       - CTRL  +   ALT +   F" & vbCrLf &
               "To Abort Flashing       - CTRL  +   ALT +   A" & vbCrLf & vbCrLf &
               "To Watch System Stats   - CTRL  +   SHIFT   +   S" & vbCrLf &
               "To Watch UART Stats     - CTRL  +   SHIFT   +   U" & vbCrLf &
               "To Enable Debug Messages    - CTRL  +   SHIFT   +   G" & vbCrLf &
               "To Disable Debug Messages   - CTRL  +   SHIFT   +   N",
               "Shortcut Key Rerefernce"
               )
    End Sub
End Class
