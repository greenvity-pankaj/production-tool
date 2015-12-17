<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class HomeScreen
    Inherits System.Windows.Forms.Form

    'Form overrides dispose to clean up the component list.
    <System.Diagnostics.DebuggerNonUserCode()> _
    Protected Overrides Sub Dispose(ByVal disposing As Boolean)
        Try
            If disposing AndAlso components IsNot Nothing Then
                components.Dispose()
            End If
        Finally
            MyBase.Dispose(disposing)
        End Try
    End Sub

    'Required by the Windows Form Designer
    Private components As System.ComponentModel.IContainer

    'NOTE: The following procedure is required by the Windows Form Designer
    'It can be modified using the Windows Form Designer.  
    'Do not modify it using the code editor.
    <System.Diagnostics.DebuggerStepThrough()> _
    Private Sub InitializeComponent()
        Dim resources As System.ComponentModel.ComponentResourceManager = New System.ComponentModel.ComponentResourceManager(GetType(HomeScreen))
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.lvClients = New System.Windows.Forms.ListBox()
        Me.grpbxTests = New System.Windows.Forms.GroupBox()
        Me.Button3 = New System.Windows.Forms.Button()
        Me.CheckBox3 = New System.Windows.Forms.CheckBox()
        Me.chkbx_RFRXSweep = New System.Windows.Forms.CheckBox()
        Me.chkbx_RFTXSweep = New System.Windows.Forms.CheckBox()
        Me.btn_RFRXSweepSetting = New System.Windows.Forms.Button()
        Me.btn_RFTXSweepSetting = New System.Windows.Forms.Button()
        Me.btnRFTXSetting = New System.Windows.Forms.Button()
        Me.btnRFRXSetting = New System.Windows.Forms.Button()
        Me.chkbxRFRX = New System.Windows.Forms.CheckBox()
        Me.btn_PLCRXSweepSettings = New System.Windows.Forms.Button()
        Me.chkbx_PLCRXSweep = New System.Windows.Forms.CheckBox()
        Me.btn_PLCTXSweepSettings = New System.Windows.Forms.Button()
        Me.chkbx_PLCTXSweep = New System.Windows.Forms.CheckBox()
        Me.btnPLCTXSettings = New System.Windows.Forms.Button()
        Me.btnPLCRXSettings = New System.Windows.Forms.Button()
        Me.chkbxRFTX = New System.Windows.Forms.CheckBox()
        Me.chkbxPLCRX = New System.Windows.Forms.CheckBox()
        Me.chkbxPLCTX = New System.Windows.Forms.CheckBox()
        Me.btnRunTest = New System.Windows.Forms.Button()
        Me.lblSessionName = New System.Windows.Forms.Label()
        Me.PictureBox1 = New System.Windows.Forms.PictureBox()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.lblSessionLocation = New System.Windows.Forms.Label()
        Me.btn_SetIP = New System.Windows.Forms.Button()
        Me.btn_ScanDevices = New System.Windows.Forms.Button()
        Me.btn_ResetDevices = New System.Windows.Forms.Button()
        Me.txtbxDummy = New System.Windows.Forms.TextBox()
        Me.bar = New System.Windows.Forms.ProgressBar()
        Me.btn_ShowResults = New System.Windows.Forms.Button()
        Me.txtbxLotNum = New System.Windows.Forms.TextBox()
        Me.txtbxSerialNum = New System.Windows.Forms.TextBox()
        Me.lblLotNum = New System.Windows.Forms.Label()
        Me.lblSerialNum = New System.Windows.Forms.Label()
        Me.btnStopTest = New System.Windows.Forms.Button()
        Me.lblResult = New System.Windows.Forms.Label()
        Me.GroupBox1.SuspendLayout()
        Me.grpbxTests.SuspendLayout()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.lvClients)
        Me.GroupBox1.Location = New System.Drawing.Point(22, 87)
        Me.GroupBox1.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Padding = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.GroupBox1.Size = New System.Drawing.Size(245, 127)
        Me.GroupBox1.TabIndex = 0
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "Connections"
        '
        'lvClients
        '
        Me.lvClients.FormattingEnabled = True
        Me.lvClients.ItemHeight = 15
        Me.lvClients.Location = New System.Drawing.Point(20, 23)
        Me.lvClients.Name = "lvClients"
        Me.lvClients.SelectionMode = System.Windows.Forms.SelectionMode.MultiExtended
        Me.lvClients.Size = New System.Drawing.Size(201, 79)
        Me.lvClients.TabIndex = 0
        '
        'grpbxTests
        '
        Me.grpbxTests.Controls.Add(Me.Button3)
        Me.grpbxTests.Controls.Add(Me.CheckBox3)
        Me.grpbxTests.Controls.Add(Me.chkbx_RFRXSweep)
        Me.grpbxTests.Controls.Add(Me.chkbx_RFTXSweep)
        Me.grpbxTests.Controls.Add(Me.btn_RFRXSweepSetting)
        Me.grpbxTests.Controls.Add(Me.btn_RFTXSweepSetting)
        Me.grpbxTests.Controls.Add(Me.btnRFTXSetting)
        Me.grpbxTests.Controls.Add(Me.btnRFRXSetting)
        Me.grpbxTests.Controls.Add(Me.chkbxRFRX)
        Me.grpbxTests.Controls.Add(Me.btn_PLCRXSweepSettings)
        Me.grpbxTests.Controls.Add(Me.chkbx_PLCRXSweep)
        Me.grpbxTests.Controls.Add(Me.btn_PLCTXSweepSettings)
        Me.grpbxTests.Controls.Add(Me.chkbx_PLCTXSweep)
        Me.grpbxTests.Controls.Add(Me.btnPLCTXSettings)
        Me.grpbxTests.Controls.Add(Me.btnPLCRXSettings)
        Me.grpbxTests.Controls.Add(Me.chkbxRFTX)
        Me.grpbxTests.Controls.Add(Me.chkbxPLCRX)
        Me.grpbxTests.Controls.Add(Me.chkbxPLCTX)
        Me.grpbxTests.Location = New System.Drawing.Point(22, 249)
        Me.grpbxTests.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.grpbxTests.Name = "grpbxTests"
        Me.grpbxTests.Padding = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.grpbxTests.Size = New System.Drawing.Size(778, 186)
        Me.grpbxTests.TabIndex = 4
        Me.grpbxTests.TabStop = False
        Me.grpbxTests.Text = "Tests"
        '
        'Button3
        '
        Me.Button3.BackgroundImage = CType(resources.GetObject("Button3.BackgroundImage"), System.Drawing.Image)
        Me.Button3.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.Button3.Enabled = False
        Me.Button3.Location = New System.Drawing.Point(737, 133)
        Me.Button3.Name = "Button3"
        Me.Button3.Size = New System.Drawing.Size(27, 24)
        Me.Button3.TabIndex = 30
        Me.Button3.UseVisualStyleBackColor = True
        '
        'CheckBox3
        '
        Me.CheckBox3.AutoSize = True
        Me.CheckBox3.Enabled = False
        Me.CheckBox3.Location = New System.Drawing.Point(571, 137)
        Me.CheckBox3.Name = "CheckBox3"
        Me.CheckBox3.Size = New System.Drawing.Size(75, 19)
        Me.CheckBox3.TabIndex = 29
        Me.CheckBox3.Text = "New Test"
        Me.CheckBox3.UseVisualStyleBackColor = True
        '
        'chkbx_RFRXSweep
        '
        Me.chkbx_RFRXSweep.AutoSize = True
        Me.chkbx_RFRXSweep.Location = New System.Drawing.Point(278, 136)
        Me.chkbx_RFRXSweep.Name = "chkbx_RFRXSweep"
        Me.chkbx_RFRXSweep.Size = New System.Drawing.Size(118, 19)
        Me.chkbx_RFRXSweep.TabIndex = 28
        Me.chkbx_RFRXSweep.Text = "RF RX Sweep Test"
        Me.chkbx_RFRXSweep.UseVisualStyleBackColor = True
        '
        'chkbx_RFTXSweep
        '
        Me.chkbx_RFTXSweep.AutoSize = True
        Me.chkbx_RFTXSweep.Location = New System.Drawing.Point(17, 138)
        Me.chkbx_RFTXSweep.Name = "chkbx_RFTXSweep"
        Me.chkbx_RFTXSweep.Size = New System.Drawing.Size(118, 19)
        Me.chkbx_RFTXSweep.TabIndex = 27
        Me.chkbx_RFTXSweep.Text = "RF TX Sweep Test"
        Me.chkbx_RFTXSweep.UseVisualStyleBackColor = True
        '
        'btn_RFRXSweepSetting
        '
        Me.btn_RFRXSweepSetting.BackgroundImage = CType(resources.GetObject("btn_RFRXSweepSetting.BackgroundImage"), System.Drawing.Image)
        Me.btn_RFRXSweepSetting.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btn_RFRXSweepSetting.Enabled = False
        Me.btn_RFRXSweepSetting.Location = New System.Drawing.Point(457, 130)
        Me.btn_RFRXSweepSetting.Name = "btn_RFRXSweepSetting"
        Me.btn_RFRXSweepSetting.Size = New System.Drawing.Size(27, 24)
        Me.btn_RFRXSweepSetting.TabIndex = 26
        Me.btn_RFRXSweepSetting.UseVisualStyleBackColor = True
        '
        'btn_RFTXSweepSetting
        '
        Me.btn_RFTXSweepSetting.BackgroundImage = CType(resources.GetObject("btn_RFTXSweepSetting.BackgroundImage"), System.Drawing.Image)
        Me.btn_RFTXSweepSetting.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btn_RFTXSweepSetting.Enabled = False
        Me.btn_RFTXSweepSetting.Location = New System.Drawing.Point(151, 131)
        Me.btn_RFTXSweepSetting.Name = "btn_RFTXSweepSetting"
        Me.btn_RFTXSweepSetting.Size = New System.Drawing.Size(27, 24)
        Me.btn_RFTXSweepSetting.TabIndex = 24
        Me.btn_RFTXSweepSetting.UseVisualStyleBackColor = True
        '
        'btnRFTXSetting
        '
        Me.btnRFTXSetting.BackgroundImage = CType(resources.GetObject("btnRFTXSetting.BackgroundImage"), System.Drawing.Image)
        Me.btnRFTXSetting.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btnRFTXSetting.Enabled = False
        Me.btnRFTXSetting.Location = New System.Drawing.Point(737, 29)
        Me.btnRFTXSetting.Name = "btnRFTXSetting"
        Me.btnRFTXSetting.Size = New System.Drawing.Size(27, 24)
        Me.btnRFTXSetting.TabIndex = 22
        Me.btnRFTXSetting.UseVisualStyleBackColor = True
        '
        'btnRFRXSetting
        '
        Me.btnRFRXSetting.BackgroundImage = CType(resources.GetObject("btnRFRXSetting.BackgroundImage"), System.Drawing.Image)
        Me.btnRFRXSetting.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btnRFRXSetting.Enabled = False
        Me.btnRFRXSetting.Location = New System.Drawing.Point(737, 82)
        Me.btnRFRXSetting.Name = "btnRFRXSetting"
        Me.btnRFRXSetting.Size = New System.Drawing.Size(27, 24)
        Me.btnRFRXSetting.TabIndex = 21
        Me.btnRFRXSetting.UseVisualStyleBackColor = True
        '
        'chkbxRFRX
        '
        Me.chkbxRFRX.AutoSize = True
        Me.chkbxRFRX.Enabled = False
        Me.chkbxRFRX.Location = New System.Drawing.Point(571, 86)
        Me.chkbxRFRX.Name = "chkbxRFRX"
        Me.chkbxRFRX.Size = New System.Drawing.Size(107, 19)
        Me.chkbxRFRX.TabIndex = 20
        Me.chkbxRFRX.Text = "RF Receive Test"
        Me.chkbxRFRX.UseVisualStyleBackColor = True
        '
        'btn_PLCRXSweepSettings
        '
        Me.btn_PLCRXSweepSettings.BackgroundImage = CType(resources.GetObject("btn_PLCRXSweepSettings.BackgroundImage"), System.Drawing.Image)
        Me.btn_PLCRXSweepSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btn_PLCRXSweepSettings.Enabled = False
        Me.btn_PLCRXSweepSettings.Location = New System.Drawing.Point(457, 80)
        Me.btn_PLCRXSweepSettings.Name = "btn_PLCRXSweepSettings"
        Me.btn_PLCRXSweepSettings.Size = New System.Drawing.Size(27, 24)
        Me.btn_PLCRXSweepSettings.TabIndex = 19
        Me.btn_PLCRXSweepSettings.UseVisualStyleBackColor = True
        '
        'chkbx_PLCRXSweep
        '
        Me.chkbx_PLCRXSweep.AutoSize = True
        Me.chkbx_PLCRXSweep.Enabled = False
        Me.chkbx_PLCRXSweep.Location = New System.Drawing.Point(278, 86)
        Me.chkbx_PLCRXSweep.Name = "chkbx_PLCRXSweep"
        Me.chkbx_PLCRXSweep.Size = New System.Drawing.Size(126, 19)
        Me.chkbx_PLCRXSweep.TabIndex = 18
        Me.chkbx_PLCRXSweep.Text = "PLC RX Sweep Test"
        Me.chkbx_PLCRXSweep.UseVisualStyleBackColor = True
        '
        'btn_PLCTXSweepSettings
        '
        Me.btn_PLCTXSweepSettings.BackgroundImage = CType(resources.GetObject("btn_PLCTXSweepSettings.BackgroundImage"), System.Drawing.Image)
        Me.btn_PLCTXSweepSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btn_PLCTXSweepSettings.Enabled = False
        Me.btn_PLCTXSweepSettings.Location = New System.Drawing.Point(151, 80)
        Me.btn_PLCTXSweepSettings.Name = "btn_PLCTXSweepSettings"
        Me.btn_PLCTXSweepSettings.Size = New System.Drawing.Size(27, 24)
        Me.btn_PLCTXSweepSettings.TabIndex = 17
        Me.btn_PLCTXSweepSettings.UseVisualStyleBackColor = True
        '
        'chkbx_PLCTXSweep
        '
        Me.chkbx_PLCTXSweep.AutoSize = True
        Me.chkbx_PLCTXSweep.Enabled = False
        Me.chkbx_PLCTXSweep.Location = New System.Drawing.Point(17, 85)
        Me.chkbx_PLCTXSweep.Name = "chkbx_PLCTXSweep"
        Me.chkbx_PLCTXSweep.Size = New System.Drawing.Size(126, 19)
        Me.chkbx_PLCTXSweep.TabIndex = 16
        Me.chkbx_PLCTXSweep.Text = "PLC TX Sweep Test"
        Me.chkbx_PLCTXSweep.UseVisualStyleBackColor = True
        '
        'btnPLCTXSettings
        '
        Me.btnPLCTXSettings.BackgroundImage = CType(resources.GetObject("btnPLCTXSettings.BackgroundImage"), System.Drawing.Image)
        Me.btnPLCTXSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btnPLCTXSettings.Location = New System.Drawing.Point(151, 29)
        Me.btnPLCTXSettings.Name = "btnPLCTXSettings"
        Me.btnPLCTXSettings.Size = New System.Drawing.Size(27, 24)
        Me.btnPLCTXSettings.TabIndex = 15
        Me.btnPLCTXSettings.UseVisualStyleBackColor = True
        '
        'btnPLCRXSettings
        '
        Me.btnPLCRXSettings.BackgroundImage = CType(resources.GetObject("btnPLCRXSettings.BackgroundImage"), System.Drawing.Image)
        Me.btnPLCRXSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btnPLCRXSettings.Location = New System.Drawing.Point(457, 28)
        Me.btnPLCRXSettings.Name = "btnPLCRXSettings"
        Me.btnPLCRXSettings.Size = New System.Drawing.Size(27, 24)
        Me.btnPLCRXSettings.TabIndex = 14
        Me.btnPLCRXSettings.UseVisualStyleBackColor = True
        '
        'chkbxRFTX
        '
        Me.chkbxRFTX.AutoSize = True
        Me.chkbxRFTX.Location = New System.Drawing.Point(571, 34)
        Me.chkbxRFTX.Name = "chkbxRFTX"
        Me.chkbxRFTX.Size = New System.Drawing.Size(114, 19)
        Me.chkbxRFTX.TabIndex = 2
        Me.chkbxRFTX.Text = "RF Transmit Test"
        Me.chkbxRFTX.UseVisualStyleBackColor = True
        '
        'chkbxPLCRX
        '
        Me.chkbxPLCRX.AutoSize = True
        Me.chkbxPLCRX.Location = New System.Drawing.Point(278, 34)
        Me.chkbxPLCRX.Name = "chkbxPLCRX"
        Me.chkbxPLCRX.Size = New System.Drawing.Size(115, 19)
        Me.chkbxPLCRX.TabIndex = 1
        Me.chkbxPLCRX.Text = "PLC Receive Test"
        Me.chkbxPLCRX.UseVisualStyleBackColor = True
        '
        'chkbxPLCTX
        '
        Me.chkbxPLCTX.AutoSize = True
        Me.chkbxPLCTX.Location = New System.Drawing.Point(18, 33)
        Me.chkbxPLCTX.Name = "chkbxPLCTX"
        Me.chkbxPLCTX.Size = New System.Drawing.Size(122, 19)
        Me.chkbxPLCTX.TabIndex = 0
        Me.chkbxPLCTX.Text = "PLC Transmit Test"
        Me.chkbxPLCTX.UseVisualStyleBackColor = True
        '
        'btnRunTest
        '
        Me.btnRunTest.Location = New System.Drawing.Point(328, 99)
        Me.btnRunTest.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.btnRunTest.Name = "btnRunTest"
        Me.btnRunTest.Size = New System.Drawing.Size(136, 47)
        Me.btnRunTest.TabIndex = 7
        Me.btnRunTest.Text = "RUN TEST"
        Me.btnRunTest.UseVisualStyleBackColor = True
        '
        'lblSessionName
        '
        Me.lblSessionName.AutoSize = True
        Me.lblSessionName.Location = New System.Drawing.Point(336, 12)
        Me.lblSessionName.Name = "lblSessionName"
        Me.lblSessionName.Size = New System.Drawing.Size(0, 15)
        Me.lblSessionName.TabIndex = 8
        '
        'PictureBox1
        '
        Me.PictureBox1.Image = CType(resources.GetObject("PictureBox1.Image"), System.Drawing.Image)
        Me.PictureBox1.Location = New System.Drawing.Point(12, 12)
        Me.PictureBox1.Name = "PictureBox1"
        Me.PictureBox1.Size = New System.Drawing.Size(318, 68)
        Me.PictureBox1.TabIndex = 9
        Me.PictureBox1.TabStop = False
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(336, 49)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(113, 15)
        Me.Label2.TabIndex = 10
        Me.Label2.Text = "Session Logged at - "
        '
        'lblSessionLocation
        '
        Me.lblSessionLocation.AutoSize = True
        Me.lblSessionLocation.Location = New System.Drawing.Point(444, 49)
        Me.lblSessionLocation.Name = "lblSessionLocation"
        Me.lblSessionLocation.Size = New System.Drawing.Size(319, 15)
        Me.lblSessionLocation.TabIndex = 11
        Me.lblSessionLocation.Text = "C:\Users\Greenvity Extra\Documents\Production Tool Logs"
        '
        'btn_SetIP
        '
        Me.btn_SetIP.Location = New System.Drawing.Point(668, 99)
        Me.btn_SetIP.Name = "btn_SetIP"
        Me.btn_SetIP.Size = New System.Drawing.Size(132, 47)
        Me.btn_SetIP.TabIndex = 17
        Me.btn_SetIP.Text = "Set Local Machine IP"
        Me.btn_SetIP.UseVisualStyleBackColor = True
        '
        'btn_ScanDevices
        '
        Me.btn_ScanDevices.Location = New System.Drawing.Point(499, 99)
        Me.btn_ScanDevices.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.btn_ScanDevices.Name = "btn_ScanDevices"
        Me.btn_ScanDevices.Size = New System.Drawing.Size(132, 47)
        Me.btn_ScanDevices.TabIndex = 18
        Me.btn_ScanDevices.Text = "SCAN DEVICES"
        Me.btn_ScanDevices.UseVisualStyleBackColor = True
        '
        'btn_ResetDevices
        '
        Me.btn_ResetDevices.Location = New System.Drawing.Point(499, 167)
        Me.btn_ResetDevices.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.btn_ResetDevices.Name = "btn_ResetDevices"
        Me.btn_ResetDevices.Size = New System.Drawing.Size(132, 47)
        Me.btn_ResetDevices.TabIndex = 19
        Me.btn_ResetDevices.Text = "RESET DEVICES"
        Me.btn_ResetDevices.UseVisualStyleBackColor = True
        '
        'txtbxDummy
        '
        Me.txtbxDummy.Location = New System.Drawing.Point(823, 110)
        Me.txtbxDummy.Multiline = True
        Me.txtbxDummy.Name = "txtbxDummy"
        Me.txtbxDummy.ScrollBars = System.Windows.Forms.ScrollBars.Both
        Me.txtbxDummy.Size = New System.Drawing.Size(305, 372)
        Me.txtbxDummy.TabIndex = 20
        '
        'bar
        '
        Me.bar.Location = New System.Drawing.Point(22, 459)
        Me.bar.Name = "bar"
        Me.bar.Size = New System.Drawing.Size(778, 23)
        Me.bar.TabIndex = 21
        '
        'btn_ShowResults
        '
        Me.btn_ShowResults.Location = New System.Drawing.Point(328, 167)
        Me.btn_ShowResults.Name = "btn_ShowResults"
        Me.btn_ShowResults.Size = New System.Drawing.Size(136, 47)
        Me.btn_ShowResults.TabIndex = 22
        Me.btn_ShowResults.Text = "SHOW RESULTS"
        Me.btn_ShowResults.UseVisualStyleBackColor = True
        '
        'txtbxLotNum
        '
        Me.txtbxLotNum.Location = New System.Drawing.Point(969, 30)
        Me.txtbxLotNum.Name = "txtbxLotNum"
        Me.txtbxLotNum.Size = New System.Drawing.Size(136, 23)
        Me.txtbxLotNum.TabIndex = 23
        Me.txtbxLotNum.Text = "0"
        Me.txtbxLotNum.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        '
        'txtbxSerialNum
        '
        Me.txtbxSerialNum.Location = New System.Drawing.Point(969, 70)
        Me.txtbxSerialNum.Name = "txtbxSerialNum"
        Me.txtbxSerialNum.Size = New System.Drawing.Size(136, 23)
        Me.txtbxSerialNum.TabIndex = 24
        Me.txtbxSerialNum.Text = "0"
        Me.txtbxSerialNum.TextAlign = System.Windows.Forms.HorizontalAlignment.Center
        '
        'lblLotNum
        '
        Me.lblLotNum.AutoSize = True
        Me.lblLotNum.Location = New System.Drawing.Point(836, 33)
        Me.lblLotNum.Name = "lblLotNum"
        Me.lblLotNum.Size = New System.Drawing.Size(82, 15)
        Me.lblLotNum.TabIndex = 25
        Me.lblLotNum.Text = "Lot Number - "
        '
        'lblSerialNum
        '
        Me.lblSerialNum.AutoSize = True
        Me.lblSerialNum.Location = New System.Drawing.Point(836, 73)
        Me.lblSerialNum.Name = "lblSerialNum"
        Me.lblSerialNum.Size = New System.Drawing.Size(127, 15)
        Me.lblSerialNum.TabIndex = 25
        Me.lblSerialNum.Text = "Board Serial Number - "
        '
        'btnStopTest
        '
        Me.btnStopTest.Location = New System.Drawing.Point(668, 167)
        Me.btnStopTest.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.btnStopTest.Name = "btnStopTest"
        Me.btnStopTest.Size = New System.Drawing.Size(132, 47)
        Me.btnStopTest.TabIndex = 26
        Me.btnStopTest.Text = "STOP TEST"
        Me.btnStopTest.UseVisualStyleBackColor = True
        '
        'lblResult
        '
        Me.lblResult.AutoSize = True
        Me.lblResult.Location = New System.Drawing.Point(665, 12)
        Me.lblResult.Name = "lblResult"
        Me.lblResult.Size = New System.Drawing.Size(0, 15)
        Me.lblResult.TabIndex = 27
        '
        'HomeScreen
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(7.0!, 15.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(1144, 507)
        Me.Controls.Add(Me.lblResult)
        Me.Controls.Add(Me.btnStopTest)
        Me.Controls.Add(Me.lblSerialNum)
        Me.Controls.Add(Me.lblLotNum)
        Me.Controls.Add(Me.txtbxSerialNum)
        Me.Controls.Add(Me.txtbxLotNum)
        Me.Controls.Add(Me.btn_ShowResults)
        Me.Controls.Add(Me.bar)
        Me.Controls.Add(Me.txtbxDummy)
        Me.Controls.Add(Me.btn_ResetDevices)
        Me.Controls.Add(Me.btn_ScanDevices)
        Me.Controls.Add(Me.btn_SetIP)
        Me.Controls.Add(Me.lblSessionLocation)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.PictureBox1)
        Me.Controls.Add(Me.lblSessionName)
        Me.Controls.Add(Me.btnRunTest)
        Me.Controls.Add(Me.grpbxTests)
        Me.Controls.Add(Me.GroupBox1)
        Me.Font = New System.Drawing.Font("Segoe UI", 9.0!, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, CType(0, Byte))
        Me.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.MaximizeBox = False
        Me.Name = "HomeScreen"
        Me.Text = "Greenvity Production Tool v2.2"
        Me.GroupBox1.ResumeLayout(False)
        Me.grpbxTests.ResumeLayout(False)
        Me.grpbxTests.PerformLayout()
        CType(Me.PictureBox1, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents GroupBox1 As System.Windows.Forms.GroupBox
    Friend WithEvents grpbxTests As System.Windows.Forms.GroupBox
    Friend WithEvents btnRunTest As System.Windows.Forms.Button
    Friend WithEvents lblSessionName As System.Windows.Forms.Label
    Friend WithEvents PictureBox1 As System.Windows.Forms.PictureBox
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents lblSessionLocation As System.Windows.Forms.Label
    Friend WithEvents chkbxRFTX As System.Windows.Forms.CheckBox
    Friend WithEvents chkbxPLCRX As System.Windows.Forms.CheckBox
    Friend WithEvents chkbxPLCTX As System.Windows.Forms.CheckBox
    Friend WithEvents btnPLCTXSettings As System.Windows.Forms.Button
    Friend WithEvents btnPLCRXSettings As System.Windows.Forms.Button
    Friend WithEvents btnRFRXSetting As System.Windows.Forms.Button
    Friend WithEvents chkbxRFRX As System.Windows.Forms.CheckBox
    Friend WithEvents btn_PLCRXSweepSettings As System.Windows.Forms.Button
    Friend WithEvents chkbx_PLCRXSweep As System.Windows.Forms.CheckBox
    Friend WithEvents btn_PLCTXSweepSettings As System.Windows.Forms.Button
    Friend WithEvents chkbx_PLCTXSweep As System.Windows.Forms.CheckBox
    Friend WithEvents btn_SetIP As System.Windows.Forms.Button
    Friend WithEvents btn_ScanDevices As System.Windows.Forms.Button
    Friend WithEvents btn_ResetDevices As System.Windows.Forms.Button
    Friend WithEvents txtbxDummy As System.Windows.Forms.TextBox
    Friend WithEvents lvClients As System.Windows.Forms.ListBox
    Friend WithEvents bar As System.Windows.Forms.ProgressBar
    Friend WithEvents btnRFTXSetting As System.Windows.Forms.Button
    Friend WithEvents btn_ShowResults As System.Windows.Forms.Button
    Friend WithEvents txtbxLotNum As System.Windows.Forms.TextBox
    Friend WithEvents txtbxSerialNum As System.Windows.Forms.TextBox
    Friend WithEvents lblLotNum As System.Windows.Forms.Label
    Friend WithEvents lblSerialNum As System.Windows.Forms.Label
    Friend WithEvents Button3 As System.Windows.Forms.Button
    Friend WithEvents CheckBox3 As System.Windows.Forms.CheckBox
    Friend WithEvents chkbx_RFRXSweep As System.Windows.Forms.CheckBox
    Friend WithEvents chkbx_RFTXSweep As System.Windows.Forms.CheckBox
    Friend WithEvents btn_RFRXSweepSetting As System.Windows.Forms.Button
    Friend WithEvents btn_RFTXSweepSetting As System.Windows.Forms.Button
    Friend WithEvents btnStopTest As System.Windows.Forms.Button
    Friend WithEvents lblResult As System.Windows.Forms.Label

End Class
