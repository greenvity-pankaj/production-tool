Imports System.Runtime.Serialization.Formatters.Binary
Imports System.Net, System.Net.Sockets, System.Text, System.Threading
Imports System.Runtime.InteropServices

Public Class spiTXTestSettings
    '   <summary>
    '       Structure Declarations for tests
    '   </summary>
#Region "Enum Declaration"

    '
    '   Security Modes
    '
    Enum eSecTestMode As Byte
        UNENCRYPTED = &H0
        ENCRYPTED = &H1             'TestParams.eks gives the key
        ALT_UNENC_NEK = &H2
        ALT_UNENC_NEK_PPEK = &H3    'alternating non_secure, eks1 (nek), eks2(ppek)
    End Enum

    '
    '   Frame transmit mode
    '
    Enum eFrmMcstMode As Byte
        HPGP_UCST = &H0
        HPGP_MCST = &H1
        HPGP_MNBCST = &H2
    End Enum

    '
    '   plid value
    '
    Enum eHpgpPlidValue As Byte
        HPGP_PLID0 = &H0
        HPGP_PLID1 = &H1
        HPGP_PLID2 = &H2
        HPGP_PLID3 = &H3
    End Enum

    '
    '   Packet Length
    '
    Enum eLenTestMode As Byte
        INC_LEN_SINGLE_ROBO = &H0   'Incremental length within any one robo mode better not mix this with alt Frm type
        INC_LEN_ALL_ROBO = &H1      'Incremental length spanning across all robo modes better not mix this with alt Frm type
        FIXED_LEN_ALT_ROBO = &H2    'One fixed length for each Robo Test Mode
        FIXED_LEN = &H3
        VARY_LEN = &H4
    End Enum

    '
    '   Robo Test Mode
    '
    Enum eRoboTestMode As Byte
        MINI_ROBO_TEST = &H0
        STD_ROBO_TEST = &H1
        HS1PB_ROBO_TEST = &H2
        HS2PB_ROBO_TEST = &H3
        HS3PB_ROBO_TEST = &H4
        HSALLPB_ROBO_TEST = &H5    'Combination of 1,2 and 3 PB HS
    End Enum

    '
    '   Frame type
    '
    Enum eFrmType As Byte
        HPGP_HW_FRMTYPE_SOUND = &H0
        HPGP_HW_FRMTYPE_MSDU = &H1
        HPGP_HW_FRMTYPE_BEACON = &H2
        HPGP_HW_FRMTYPE_MGMT = &H3
        HPGP_HW_FRMTYPE_RTS = &H4
        HPGP_HW_FRMTYPE_CTS = &H5
    End Enum

#End Region

    '   <summary>
    '       Structure Declarations for tests
    '   </summary>
#Region "Structure Declerations"

    <StructLayout(LayoutKind.Explicit, Size:=37, pack:=1)> _
    Public Structure _sPlcSimTxTestParams
        '   Security
        <FieldOffset(0)> <MarshalAsAttribute(UnmanagedType.U1)> Public eks As Byte
        <FieldOffset(1)> <MarshalAsAttribute(UnmanagedType.U1)> Public secTestMode As Byte

        'frame type
        <FieldOffset(2)> <MarshalAsAttribute(UnmanagedType.U1)> Public frmType As Byte
        <FieldOffset(3)> <MarshalAsAttribute(UnmanagedType.U1)> Public altFrmTypeTest As Byte           'overrides frmType

        'ucst/mcst/bcst
        <FieldOffset(4)> <MarshalAsAttribute(UnmanagedType.U1)> Public mcstMode As Byte
        <FieldOffset(5)> <MarshalAsAttribute(UnmanagedType.U1)> Public altMcstTest As Byte

        'plid
        <FieldOffset(6)> <MarshalAsAttribute(UnmanagedType.U1)> Public plid As Byte
        <FieldOffset(7)> <MarshalAsAttribute(UnmanagedType.U1)> Public altPlidTest As Byte              'overrides plid

        'offset descLen
        <FieldOffset(8)> <MarshalAsAttribute(UnmanagedType.U1)> Public offsetDW As Byte
        <FieldOffset(9)> <MarshalAsAttribute(UnmanagedType.U1)> Public descLen As Byte
        <FieldOffset(10)> <MarshalAsAttribute(UnmanagedType.U1)> Public altOffsetDescLenTest As Byte

        'robo mode/ lengths
        <FieldOffset(11)> <MarshalAsAttribute(UnmanagedType.U1)> Public stdModeSel As Byte               'for frm lens b/w Mini Robo Max & STD/2PB HS Max
        <FieldOffset(12)> <MarshalAsAttribute(UnmanagedType.U1)> Public lenTestMode As Byte
        <FieldOffset(13)> <MarshalAsAttribute(UnmanagedType.U1)> Public roboTestMode As Byte             'for single robo inc len test

        <FieldOffset(14)> <MarshalAsAttribute(UnmanagedType.U1)> Public contMode As Byte
        <FieldOffset(15)> <MarshalAsAttribute(UnmanagedType.U2)> Public frmLen As UShort
        <FieldOffset(17)> <MarshalAsAttribute(UnmanagedType.U4)> Public numFrames As UInteger
        <FieldOffset(21)> <MarshalAsAttribute(UnmanagedType.U4)> Public delay As UInteger
        <FieldOffset(25)> <MarshalAsAttribute(UnmanagedType.U1)> Public dt_av As Byte
        <FieldOffset(26)> <MarshalAsAttribute(UnmanagedType.U1)> Public src As Byte                      'Sound Reason Code
        <FieldOffset(27)> <MarshalAsAttribute(UnmanagedType.U1)> Public saf As Byte
        <FieldOffset(28)> <MarshalAsAttribute(UnmanagedType.U1)> Public scf As Byte
        <FieldOffset(29)> <MarshalAsAttribute(UnmanagedType.U1)> Public plcMultiPktTest As Byte
        <FieldOffset(30)> <MarshalAsAttribute(UnmanagedType.U1)> Public dbc As Byte
        <FieldOffset(31)> <MarshalAsAttribute(UnmanagedType.U1)> Public pattern As Byte

        <FieldOffset(32)> <MarshalAsAttribute(UnmanagedType.U1)> Public snid As Byte
        <FieldOffset(33)> <MarshalAsAttribute(UnmanagedType.U1)> Public dtei As Byte
        <FieldOffset(34)> <MarshalAsAttribute(UnmanagedType.U1)> Public stei As Byte

        <FieldOffset(35)> <MarshalAsAttribute(UnmanagedType.U1)> Public txpowermode As Byte
        <FieldOffset(36)> <MarshalAsAttribute(UnmanagedType.U1)> Public ermode As Byte
    End Structure

#End Region

    '   Structure Variable
    Private txTest As _sPlcSimTxTestParams  'local
    Public Const HYBRII_CELLBUF_SIZE As Integer = 128

    '   <summary>
    '       Assigning different parameter values to the structure
    '   </summary>
#Region "Assign parameter values"
    '
    '   Assign frame type
    '
    Private Sub cmbbxFrmType_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbxFrmType.SelectedIndexChanged
        txTest.frmType = CType([Enum].Parse(GetType(eFrmType), cmbbxFrmType.Text), eFrmType)
    End Sub
    '
    '   Assign security test mode   
    '
    Private Sub cmbbxSecTestMode_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbxSecTestMode.SelectedIndexChanged

        If eSecTestMode.UNENCRYPTED = CType([Enum].Parse(GetType(eSecTestMode), cmbbxSecTestMode.Text), eSecTestMode) Then
            cmbbx_Eks.Items.Clear()
            For i As Integer = 0 To 7
                cmbbx_Eks.Items.Add(i)
            Next
            cmbbx_Eks.Items.Add("F")
            cmbbx_Eks.Enabled = False
            cmbbx_Eks.SelectedIndex = cmbbx_Eks.Items.Count - 1
            txTest.secTestMode = 15
        Else
            cmbbx_Eks.Enabled = True
            txTest.secTestMode = CType([Enum].Parse(GetType(eSecTestMode), cmbbxSecTestMode.Text), eSecTestMode)
            cmbbx_Eks.Items.Clear()
            For i As Integer = 0 To 7
                cmbbx_Eks.Items.Add(i)
            Next
            cmbbx_Eks.SelectedIndex = 0
        End If

    End Sub
    '
    '   Assign length test mode
    '
    Private Sub cmbbxLenTestMode_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbxLenTestMode.SelectedIndexChanged
        txTest.lenTestMode = CType([Enum].Parse(GetType(eLenTestMode), cmbbxLenTestMode.Text), eLenTestMode)
    End Sub
    '
    '   Assign continuous mode
    '    
    Private Sub chkbx_ContMode_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_ContMode.CheckedChanged
        If chkbx_ContMode.Checked Then
            txTest.contMode = &H1
        Else
            txTest.contMode = &H0
        End If
    End Sub
    '
    '   Assign Broadcast mode
    '
    Private Sub chkbx_BCastMode_CheckedChanged(sender As Object, e As EventArgs) Handles chkbx_BCastMode.CheckedChanged
        If chkbx_BCastMode.Checked Then
            txTest.mcstMode = &H1
        Else
            txTest.mcstMode = &H0
        End If
    End Sub
    ''
    ''   Assign Frame length
    ''
    'Private Sub txtbx_FrmLen_TextChanged(sender As Object, e As EventArgs) Handles txtbx_FrmLen.TextChanged
    '    txTest.frmLen = txtbx_FrmLen.Text
    'End Sub
    ''
    ''   Assign inter frame delay
    ''
    'Private Sub txtbx_InterFrmDelay_TextChanged(sender As Object, e As EventArgs) Handles txtbx_InterFrmDelay.TextChanged
    '    txTest.delay = txtbx_InterFrmDelay.Text
    'End Sub
    ''
    ''   Assign number of frames
    ''
    'Private Sub txtbx_NumOfFrms_TextChanged(sender As Object, e As EventArgs) Handles txtbx_NumOfFrms.TextChanged
    '    txTest.numFrames = txtbx_NumOfFrms.Text
    'End Sub
    '
    '   Assign key index
    '
    Private Sub cmbbx_Eks_SelectedIndexChanged(sender As Object, e As EventArgs) Handles cmbbx_Eks.SelectedIndexChanged
        If "F" = cmbbx_Eks.Text Then
            txTest.eks = 15
        Else
            txTest.eks = cmbbx_Eks.Text
        End If
    End Sub

#End Region

    '
    '   Save all the parametes
    '
    Public Sub btn_Save_Click(sender As Object, e As EventArgs) Handles btn_Save.Click

        '   Assign text box values
        txTest.frmLen = txtbx_FrmLen.Text
        txTest.delay = txtbx_InterFrmDelay.Text
        txTest.numFrames = txtbx_NumOfFrms.Text


        '   Default and not open to user
        txTest.snid = 1
        txTest.dtei = 2
        txTest.stei = 3

        txTest.descLen = HYBRII_CELLBUF_SIZE

        If HomeScreen.testParamsFor = HomeScreen.tests.TEST_ID_PLC_TX Then
            HomeScreen.plcTXparams = txTest
            HomeScreen.userPLCTXPramasSet = True
        ElseIf HomeScreen.testParamsFor = HomeScreen.tests.TEST_ID_PLC_RX Then
            HomeScreen.plcRXparams = txTest
            HomeScreen.userPLCRXPramasSet = True
        End If

        'HomeScreen.gtxTest = txTest
        txTest = Nothing
        Me.Dispose()
    End Sub
    '
    '   Form dispose
    '
    Private Sub btn_Cancle_Click(sender As Object, e As EventArgs) Handles btn_Cancle.Click
        txTest = Nothing
        Threading.Thread.Sleep(10)
        Me.Dispose()
    End Sub
    '
    '   Main Form Init
    '
    Private Sub spiTXTestSettings_Load(sender As Object, e As EventArgs) Handles MyBase.Load
        cmbbxFrmType.SelectedIndex = 0
        cmbbxLenTestMode.SelectedIndex = 0
        cmbbxSecTestMode.SelectedIndex = 0
        cmbbxFrmType.SelectedIndex = 0

        '   Disable cont mode
        cmbbx_Eks.Enabled = False
        chkbx_ContMode.Enabled = False

        If HomeScreen.chkbx_txSweep.Checked = True Then

            txtbx_FrmLen.AppendText("100")
            txtbx_InterFrmDelay.AppendText("4")
            txtbx_NumOfFrms.AppendText("1000")

            chkbx_BCastMode.Enabled = False

            cmbbx_Eks.SelectedIndex = 0
            cmbbxLenTestMode.SelectedIndex = 3
            cmbbxSecTestMode.SelectedIndex = 1

            cmbbxFrmType.Enabled = False
            cmbbxLenTestMode.Enabled = False
            cmbbxSecTestMode.Enabled = False
            cmbbxFrmType.Enabled = False

            txtbx_FrmLen.Enabled = False
            txtbx_InterFrmDelay.Enabled = False
            txtbx_NumOfFrms.Enabled = False

            btn_Save.Enabled = False
            btn_Cancle.Text = "OK"

        ElseIf HomeScreen.chkbx_rxSweep.Checked = True Then

            txtbx_FrmLen.AppendText("100")
            txtbx_InterFrmDelay.AppendText("4")
            txtbx_NumOfFrms.AppendText("1000")

            chkbx_BCastMode.Enabled = False

            cmbbx_Eks.SelectedIndex = 0
            cmbbxLenTestMode.SelectedIndex = 3
            cmbbxSecTestMode.SelectedIndex = 1

            cmbbxFrmType.Enabled = False
            cmbbxLenTestMode.Enabled = False
            cmbbxSecTestMode.Enabled = False
            cmbbxFrmType.Enabled = False

            txtbx_FrmLen.Enabled = False
            txtbx_InterFrmDelay.Enabled = False
            txtbx_NumOfFrms.Enabled = False

            btn_Save.Enabled = False
            btn_Cancle.Text = "OK"

        End If
    End Sub

End Class