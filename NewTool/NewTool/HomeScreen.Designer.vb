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
        Me.Button1 = New System.Windows.Forms.Button()
        Me.Button9 = New System.Windows.Forms.Button()
        Me.CheckBox6 = New System.Windows.Forms.CheckBox()
        Me.btn_rxSweepSettings = New System.Windows.Forms.Button()
        Me.chkbx_rxSweep = New System.Windows.Forms.CheckBox()
        Me.btn_txSweepSettings = New System.Windows.Forms.Button()
        Me.chkbx_txSweep = New System.Windows.Forms.CheckBox()
        Me.btnSettingsPLCTX = New System.Windows.Forms.Button()
        Me.btnSettingsPLCRX = New System.Windows.Forms.Button()
        Me.Checkbox = New System.Windows.Forms.CheckBox()
        Me.chkbxPLCRX = New System.Windows.Forms.CheckBox()
        Me.chkbxPLCTX = New System.Windows.Forms.CheckBox()
        Me.btnRunTest = New System.Windows.Forms.Button()
        Me.lblSessionName = New System.Windows.Forms.Label()
        Me.PictureBox1 = New System.Windows.Forms.PictureBox()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.lblSessionLocation = New System.Windows.Forms.Label()
        Me.btnBrowseLocation = New System.Windows.Forms.Button()
        Me.btn_SetIP = New System.Windows.Forms.Button()
        Me.btn_ScanDevices = New System.Windows.Forms.Button()
        Me.btn_ResetDevices = New System.Windows.Forms.Button()
        Me.txtbxDummy = New System.Windows.Forms.TextBox()
        Me.bar = New System.Windows.Forms.ProgressBar()
        Me.btn_ShowResults = New System.Windows.Forms.Button()
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
        Me.grpbxTests.Controls.Add(Me.Button1)
        Me.grpbxTests.Controls.Add(Me.Button9)
        Me.grpbxTests.Controls.Add(Me.CheckBox6)
        Me.grpbxTests.Controls.Add(Me.btn_rxSweepSettings)
        Me.grpbxTests.Controls.Add(Me.chkbx_rxSweep)
        Me.grpbxTests.Controls.Add(Me.btn_txSweepSettings)
        Me.grpbxTests.Controls.Add(Me.chkbx_txSweep)
        Me.grpbxTests.Controls.Add(Me.btnSettingsPLCTX)
        Me.grpbxTests.Controls.Add(Me.btnSettingsPLCRX)
        Me.grpbxTests.Controls.Add(Me.Checkbox)
        Me.grpbxTests.Controls.Add(Me.chkbxPLCRX)
        Me.grpbxTests.Controls.Add(Me.chkbxPLCTX)
        Me.grpbxTests.Location = New System.Drawing.Point(22, 249)
        Me.grpbxTests.Margin = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.grpbxTests.Name = "grpbxTests"
        Me.grpbxTests.Padding = New System.Windows.Forms.Padding(3, 4, 3, 4)
        Me.grpbxTests.Size = New System.Drawing.Size(800, 134)
        Me.grpbxTests.TabIndex = 4
        Me.grpbxTests.TabStop = False
        Me.grpbxTests.Text = "Tests"
        '
        'Button1
        '
        Me.Button1.BackgroundImage = CType(resources.GetObject("Button1.BackgroundImage"), System.Drawing.Image)
        Me.Button1.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.Button1.Enabled = False
        Me.Button1.Location = New System.Drawing.Point(751, 28)
        Me.Button1.Name = "Button1"
        Me.Button1.Size = New System.Drawing.Size(27, 24)
        Me.Button1.TabIndex = 22
        Me.Button1.UseVisualStyleBackColor = True
        '
        'Button9
        '
        Me.Button9.BackgroundImage = CType(resources.GetObject("Button9.BackgroundImage"), System.Drawing.Image)
        Me.Button9.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.Button9.Enabled = False
        Me.Button9.Location = New System.Drawing.Point(751, 81)
        Me.Button9.Name = "Button9"
        Me.Button9.Size = New System.Drawing.Size(27, 24)
        Me.Button9.TabIndex = 21
        Me.Button9.UseVisualStyleBackColor = True
        '
        'CheckBox6
        '
        Me.CheckBox6.AutoSize = True
        Me.CheckBox6.Enabled = False
        Me.CheckBox6.Location = New System.Drawing.Point(549, 84)
        Me.CheckBox6.Name = "CheckBox6"
        Me.CheckBox6.Size = New System.Drawing.Size(113, 19)
        Me.CheckBox6.TabIndex = 20
        Me.CheckBox6.Text = "New Device Test"
        Me.CheckBox6.UseVisualStyleBackColor = True
        '
        'btn_rxSweepSettings
        '
        Me.btn_rxSweepSettings.BackgroundImage = CType(resources.GetObject("btn_rxSweepSettings.BackgroundImage"), System.Drawing.Image)
        Me.btn_rxSweepSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btn_rxSweepSettings.Enabled = False
        Me.btn_rxSweepSettings.Location = New System.Drawing.Point(440, 80)
        Me.btn_rxSweepSettings.Name = "btn_rxSweepSettings"
        Me.btn_rxSweepSettings.Size = New System.Drawing.Size(27, 24)
        Me.btn_rxSweepSettings.TabIndex = 19
        Me.btn_rxSweepSettings.UseVisualStyleBackColor = True
        '
        'chkbx_rxSweep
        '
        Me.chkbx_rxSweep.AutoSize = True
        Me.chkbx_rxSweep.Enabled = False
        Me.chkbx_rxSweep.Location = New System.Drawing.Point(267, 86)
        Me.chkbx_rxSweep.Name = "chkbx_rxSweep"
        Me.chkbx_rxSweep.Size = New System.Drawing.Size(102, 19)
        Me.chkbx_rxSweep.TabIndex = 18
        Me.chkbx_rxSweep.Text = "RX Sweep Test"
        Me.chkbx_rxSweep.UseVisualStyleBackColor = True
        '
        'btn_txSweepSettings
        '
        Me.btn_txSweepSettings.BackgroundImage = CType(resources.GetObject("btn_txSweepSettings.BackgroundImage"), System.Drawing.Image)
        Me.btn_txSweepSettings.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btn_txSweepSettings.Enabled = False
        Me.btn_txSweepSettings.Location = New System.Drawing.Point(151, 80)
        Me.btn_txSweepSettings.Name = "btn_txSweepSettings"
        Me.btn_txSweepSettings.Size = New System.Drawing.Size(27, 24)
        Me.btn_txSweepSettings.TabIndex = 17
        Me.btn_txSweepSettings.UseVisualStyleBackColor = True
        '
        'chkbx_txSweep
        '
        Me.chkbx_txSweep.AutoSize = True
        Me.chkbx_txSweep.Enabled = False
        Me.chkbx_txSweep.Location = New System.Drawing.Point(17, 85)
        Me.chkbx_txSweep.Name = "chkbx_txSweep"
        Me.chkbx_txSweep.Size = New System.Drawing.Size(102, 19)
        Me.chkbx_txSweep.TabIndex = 16
        Me.chkbx_txSweep.Text = "TX Sweep Test"
        Me.chkbx_txSweep.UseVisualStyleBackColor = True
        '
        'btnSettingsPLCTX
        '
        Me.btnSettingsPLCTX.BackgroundImage = CType(resources.GetObject("btnSettingsPLCTX.BackgroundImage"), System.Drawing.Image)
        Me.btnSettingsPLCTX.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btnSettingsPLCTX.Location = New System.Drawing.Point(151, 29)
        Me.btnSettingsPLCTX.Name = "btnSettingsPLCTX"
        Me.btnSettingsPLCTX.Size = New System.Drawing.Size(27, 24)
        Me.btnSettingsPLCTX.TabIndex = 15
        Me.btnSettingsPLCTX.UseVisualStyleBackColor = True
        '
        'btnSettingsPLCRX
        '
        Me.btnSettingsPLCRX.BackgroundImage = CType(resources.GetObject("btnSettingsPLCRX.BackgroundImage"), System.Drawing.Image)
        Me.btnSettingsPLCRX.BackgroundImageLayout = System.Windows.Forms.ImageLayout.Center
        Me.btnSettingsPLCRX.Location = New System.Drawing.Point(440, 28)
        Me.btnSettingsPLCRX.Name = "btnSettingsPLCRX"
        Me.btnSettingsPLCRX.Size = New System.Drawing.Size(27, 24)
        Me.btnSettingsPLCRX.TabIndex = 14
        Me.btnSettingsPLCRX.UseVisualStyleBackColor = True
        '
        'Checkbox
        '
        Me.Checkbox.AutoSize = True
        Me.Checkbox.Location = New System.Drawing.Point(549, 32)
        Me.Checkbox.Name = "Checkbox"
        Me.Checkbox.Size = New System.Drawing.Size(113, 19)
        Me.Checkbox.TabIndex = 2
        Me.Checkbox.Text = "New Device Test"
        Me.Checkbox.UseVisualStyleBackColor = True
        '
        'chkbxPLCRX
        '
        Me.chkbxPLCRX.AutoSize = True
        Me.chkbxPLCRX.Location = New System.Drawing.Point(267, 34)
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
        Me.lblSessionLocation.Size = New System.Drawing.Size(283, 15)
        Me.lblSessionLocation.TabIndex = 11
        Me.lblSessionLocation.Text = "C:\Users\Greenvity Extra\Documents\Test Results.txt"
        '
        'btnBrowseLocation
        '
        Me.btnBrowseLocation.Location = New System.Drawing.Point(755, 45)
        Me.btnBrowseLocation.Name = "btnBrowseLocation"
        Me.btnBrowseLocation.Size = New System.Drawing.Size(45, 23)
        Me.btnBrowseLocation.TabIndex = 12
        Me.btnBrowseLocation.Text = "..."
        Me.btnBrowseLocation.UseVisualStyleBackColor = True
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
        Me.txtbxDummy.Location = New System.Drawing.Point(839, 45)
        Me.txtbxDummy.Multiline = True
        Me.txtbxDummy.Name = "txtbxDummy"
        Me.txtbxDummy.ScrollBars = System.Windows.Forms.ScrollBars.Both
        Me.txtbxDummy.Size = New System.Drawing.Size(305, 386)
        Me.txtbxDummy.TabIndex = 20
        '
        'bar
        '
        Me.bar.Location = New System.Drawing.Point(22, 408)
        Me.bar.Name = "bar"
        Me.bar.Size = New System.Drawing.Size(800, 23)
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
        'HomeScreen
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(7.0!, 15.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(1156, 460)
        Me.Controls.Add(Me.btn_ShowResults)
        Me.Controls.Add(Me.bar)
        Me.Controls.Add(Me.txtbxDummy)
        Me.Controls.Add(Me.btn_ResetDevices)
        Me.Controls.Add(Me.btn_ScanDevices)
        Me.Controls.Add(Me.btn_SetIP)
        Me.Controls.Add(Me.btnBrowseLocation)
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
        Me.Text = "Greenvity Production Tool"
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
    Friend WithEvents btnBrowseLocation As System.Windows.Forms.Button
    Friend WithEvents Checkbox As System.Windows.Forms.CheckBox
    Friend WithEvents chkbxPLCRX As System.Windows.Forms.CheckBox
    Friend WithEvents chkbxPLCTX As System.Windows.Forms.CheckBox
    Friend WithEvents btnSettingsPLCTX As System.Windows.Forms.Button
    Friend WithEvents btnSettingsPLCRX As System.Windows.Forms.Button
    Friend WithEvents Button9 As System.Windows.Forms.Button
    Friend WithEvents CheckBox6 As System.Windows.Forms.CheckBox
    Friend WithEvents btn_rxSweepSettings As System.Windows.Forms.Button
    Friend WithEvents chkbx_rxSweep As System.Windows.Forms.CheckBox
    Friend WithEvents btn_txSweepSettings As System.Windows.Forms.Button
    Friend WithEvents chkbx_txSweep As System.Windows.Forms.CheckBox
    Friend WithEvents btn_SetIP As System.Windows.Forms.Button
    Friend WithEvents btn_ScanDevices As System.Windows.Forms.Button
    Friend WithEvents btn_ResetDevices As System.Windows.Forms.Button
    Friend WithEvents txtbxDummy As System.Windows.Forms.TextBox
    Friend WithEvents lvClients As System.Windows.Forms.ListBox
    Friend WithEvents bar As System.Windows.Forms.ProgressBar
    Friend WithEvents Button1 As System.Windows.Forms.Button
    Friend WithEvents btn_ShowResults As System.Windows.Forms.Button

End Class
