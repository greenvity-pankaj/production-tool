<Global.Microsoft.VisualBasic.CompilerServices.DesignerGenerated()> _
Partial Class Form1
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
        Me.components = New System.ComponentModel.Container()
        Me.ProgressBar1 = New System.Windows.Forms.ProgressBar()
        Me.TextBox1 = New System.Windows.Forms.TextBox()
        Me.Timer1 = New System.Windows.Forms.Timer(Me.components)
        Me.OpenFileDialog1 = New System.Windows.Forms.OpenFileDialog()
        Me.MenuStrip1 = New System.Windows.Forms.MenuStrip()
        Me.ToolStripMenuItem1 = New System.Windows.Forms.ToolStripMenuItem()
        Me.OpenToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.SaveToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.CloseToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.EditToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.DeviceToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.AboutToolStripMenuItem = New System.Windows.Forms.ToolStripMenuItem()
        Me.SaveFileDialog1 = New System.Windows.Forms.SaveFileDialog()
        Me.lbl_file = New System.Windows.Forms.Label()
        Me.Label2 = New System.Windows.Forms.Label()
        Me.GroupBox1 = New System.Windows.Forms.GroupBox()
        Me.chkbx_text_case = New System.Windows.Forms.CheckBox()
        Me.chkbx_8x1 = New System.Windows.Forms.CheckBox()
        Me.chkbx_buff_init = New System.Windows.Forms.CheckBox()
        Me.btn_open = New System.Windows.Forms.Button()
        Me.btn_hexview = New System.Windows.Forms.Button()
        'Me.At_isp_device1 = New USB_Atmel_Device_ISP_Programmer.at_isp_device()
        Me.btn_save_text = New System.Windows.Forms.Button()
        Me.SaveFileDialog2 = New System.Windows.Forms.SaveFileDialog()
        Me.MenuStrip1.SuspendLayout()
        Me.GroupBox1.SuspendLayout()
        'CType(Me.At_isp_device1, System.ComponentModel.ISupportInitialize).BeginInit()
        Me.SuspendLayout()
        '
        'ProgressBar1
        '
        Me.ProgressBar1.Location = New System.Drawing.Point(161, 228)
        Me.ProgressBar1.Name = "ProgressBar1"
        Me.ProgressBar1.Size = New System.Drawing.Size(347, 23)
        Me.ProgressBar1.TabIndex = 0
        '
        'TextBox1
        '
        Me.TextBox1.Location = New System.Drawing.Point(26, 23)
        Me.TextBox1.Multiline = True
        Me.TextBox1.Name = "TextBox1"
        Me.TextBox1.ScrollBars = System.Windows.Forms.ScrollBars.Vertical
        Me.TextBox1.Size = New System.Drawing.Size(596, 199)
        Me.TextBox1.TabIndex = 1
        '
        'OpenFileDialog1
        '
        Me.OpenFileDialog1.Filter = """Hex Files(.hex)|*.hex|Bin Files(.bin)|*.bin|All Files(*.*)|*.*"""
        '
        'MenuStrip1
        '
        Me.MenuStrip1.Items.AddRange(New System.Windows.Forms.ToolStripItem() {Me.ToolStripMenuItem1, Me.EditToolStripMenuItem, Me.DeviceToolStripMenuItem, Me.AboutToolStripMenuItem})
        Me.MenuStrip1.Location = New System.Drawing.Point(0, 0)
        Me.MenuStrip1.Name = "MenuStrip1"
        Me.MenuStrip1.Size = New System.Drawing.Size(846, 24)
        Me.MenuStrip1.TabIndex = 8
        Me.MenuStrip1.Text = "MenuStrip1"
        '
        'ToolStripMenuItem1
        '
        Me.ToolStripMenuItem1.DropDownItems.AddRange(New System.Windows.Forms.ToolStripItem() {Me.OpenToolStripMenuItem, Me.SaveToolStripMenuItem, Me.CloseToolStripMenuItem})
        Me.ToolStripMenuItem1.Name = "ToolStripMenuItem1"
        Me.ToolStripMenuItem1.Size = New System.Drawing.Size(37, 20)
        Me.ToolStripMenuItem1.Text = "File"
        '
        'OpenToolStripMenuItem
        '
        Me.OpenToolStripMenuItem.Name = "OpenToolStripMenuItem"
        Me.OpenToolStripMenuItem.Size = New System.Drawing.Size(103, 22)
        Me.OpenToolStripMenuItem.Text = "Open"
        '
        'SaveToolStripMenuItem
        '
        Me.SaveToolStripMenuItem.Name = "SaveToolStripMenuItem"
        Me.SaveToolStripMenuItem.Size = New System.Drawing.Size(103, 22)
        Me.SaveToolStripMenuItem.Text = "Save"
        '
        'CloseToolStripMenuItem
        '
        Me.CloseToolStripMenuItem.Name = "CloseToolStripMenuItem"
        Me.CloseToolStripMenuItem.Size = New System.Drawing.Size(103, 22)
        Me.CloseToolStripMenuItem.Text = "Close"
        '
        'EditToolStripMenuItem
        '
        Me.EditToolStripMenuItem.Name = "EditToolStripMenuItem"
        Me.EditToolStripMenuItem.Size = New System.Drawing.Size(39, 20)
        Me.EditToolStripMenuItem.Text = "Edit"
        '
        'DeviceToolStripMenuItem
        '
        Me.DeviceToolStripMenuItem.Name = "DeviceToolStripMenuItem"
        Me.DeviceToolStripMenuItem.Size = New System.Drawing.Size(54, 20)
        Me.DeviceToolStripMenuItem.Text = "Device"
        '
        'AboutToolStripMenuItem
        '
        Me.AboutToolStripMenuItem.Name = "AboutToolStripMenuItem"
        Me.AboutToolStripMenuItem.Size = New System.Drawing.Size(52, 20)
        Me.AboutToolStripMenuItem.Text = "About"
        '
        'SaveFileDialog1
        '
        Me.SaveFileDialog1.Filter = """Hex Files(.hex)|*.hex|Bin Files(.bin)|*.bin|All Files(*.*)|*.*"""
        '
        'lbl_file
        '
        Me.lbl_file.AutoSize = True
        Me.lbl_file.Location = New System.Drawing.Point(28, 338)
        Me.lbl_file.Name = "lbl_file"
        Me.lbl_file.Size = New System.Drawing.Size(26, 13)
        Me.lbl_file.TabIndex = 9
        Me.lbl_file.Text = "File:"
        '
        'Label2
        '
        Me.Label2.AutoSize = True
        Me.Label2.Location = New System.Drawing.Point(28, 362)
        Me.Label2.Name = "Label2"
        Me.Label2.Size = New System.Drawing.Size(40, 13)
        Me.Label2.TabIndex = 10
        Me.Label2.Text = "Status:"
        '
        'GroupBox1
        '
        Me.GroupBox1.Controls.Add(Me.chkbx_text_case)
        Me.GroupBox1.Controls.Add(Me.chkbx_8x1)
        Me.GroupBox1.Controls.Add(Me.chkbx_buff_init)
        Me.GroupBox1.Controls.Add(Me.ProgressBar1)
        Me.GroupBox1.Controls.Add(Me.TextBox1)
        Me.GroupBox1.Location = New System.Drawing.Point(22, 69)
        Me.GroupBox1.Name = "GroupBox1"
        Me.GroupBox1.Size = New System.Drawing.Size(812, 257)
        Me.GroupBox1.TabIndex = 11
        Me.GroupBox1.TabStop = False
        Me.GroupBox1.Text = "Runtime Viewer"
        '
        'chkbx_text_case
        '
        Me.chkbx_text_case.AutoSize = True
        Me.chkbx_text_case.Location = New System.Drawing.Point(657, 80)
        Me.chkbx_text_case.Name = "chkbx_text_case"
        Me.chkbx_text_case.Size = New System.Drawing.Size(106, 17)
        Me.chkbx_text_case.TabIndex = 11
        Me.chkbx_text_case.Text = "Text Case Upper"
        Me.chkbx_text_case.UseVisualStyleBackColor = True
        '
        'chkbx_8x1
        '
        Me.chkbx_8x1.AutoSize = True
        Me.chkbx_8x1.Location = New System.Drawing.Point(657, 125)
        Me.chkbx_8x1.Name = "chkbx_8x1"
        Me.chkbx_8x1.Size = New System.Drawing.Size(78, 17)
        Me.chkbx_8x1.TabIndex = 10
        Me.chkbx_8x1.Text = "8x1 Format"
        Me.chkbx_8x1.UseVisualStyleBackColor = True
        '
        'chkbx_buff_init
        '
        Me.chkbx_buff_init.AutoSize = True
        Me.chkbx_buff_init.Checked = True
        Me.chkbx_buff_init.CheckState = System.Windows.Forms.CheckState.Checked
        Me.chkbx_buff_init.Location = New System.Drawing.Point(657, 103)
        Me.chkbx_buff_init.Name = "chkbx_buff_init"
        Me.chkbx_buff_init.Size = New System.Drawing.Size(102, 17)
        Me.chkbx_buff_init.TabIndex = 9
        Me.chkbx_buff_init.Text = "Buffer Init with 0"
        Me.chkbx_buff_init.UseVisualStyleBackColor = True
        '
        'btn_open
        '
        Me.btn_open.Location = New System.Drawing.Point(21, 36)
        Me.btn_open.Name = "btn_open"
        Me.btn_open.Size = New System.Drawing.Size(75, 23)
        Me.btn_open.TabIndex = 12
        Me.btn_open.Text = "Open"
        Me.btn_open.UseVisualStyleBackColor = True
        '
        'btn_hexview
        '
        Me.btn_hexview.Location = New System.Drawing.Point(102, 36)
        Me.btn_hexview.Name = "btn_hexview"
        Me.btn_hexview.Size = New System.Drawing.Size(75, 23)
        Me.btn_hexview.TabIndex = 13
        Me.btn_hexview.Text = "Hex View"
        Me.btn_hexview.UseVisualStyleBackColor = True
        '
        'At_isp_device1
        '
        'Me.At_isp_device1.DataSetName = "at_isp_device"
        'Me.At_isp_device1.SchemaSerializationMode = System.Data.SchemaSerializationMode.IncludeSchema
        '
        'btn_save_text
        '
        Me.btn_save_text.Location = New System.Drawing.Point(183, 36)
        Me.btn_save_text.Name = "btn_save_text"
        Me.btn_save_text.Size = New System.Drawing.Size(75, 23)
        Me.btn_save_text.TabIndex = 20
        Me.btn_save_text.Text = "Save Text"
        Me.btn_save_text.UseVisualStyleBackColor = True
        '
        'SaveFileDialog2
        '
        Me.SaveFileDialog2.Filter = """Text Files(.txt)|*.txt|All Files(*.*)|*.*"""
        '
        'Form1
        '
        Me.AutoScaleDimensions = New System.Drawing.SizeF(6.0!, 13.0!)
        Me.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font
        Me.ClientSize = New System.Drawing.Size(846, 384)
        Me.Controls.Add(Me.btn_save_text)
        Me.Controls.Add(Me.btn_hexview)
        Me.Controls.Add(Me.btn_open)
        Me.Controls.Add(Me.GroupBox1)
        Me.Controls.Add(Me.Label2)
        Me.Controls.Add(Me.lbl_file)
        Me.Controls.Add(Me.MenuStrip1)
        Me.MainMenuStrip = Me.MenuStrip1
        Me.Name = "Form1"
        Me.Text = "Hex to Text Converter"
        Me.MenuStrip1.ResumeLayout(False)
        Me.MenuStrip1.PerformLayout()
        Me.GroupBox1.ResumeLayout(False)
        Me.GroupBox1.PerformLayout()
        ' CType(Me.At_isp_device1, System.ComponentModel.ISupportInitialize).EndInit()
        Me.ResumeLayout(False)
        Me.PerformLayout()

    End Sub
    Friend WithEvents ProgressBar1 As System.Windows.Forms.ProgressBar
    Friend WithEvents TextBox1 As System.Windows.Forms.TextBox
    Friend WithEvents Timer1 As System.Windows.Forms.Timer
    Friend WithEvents OpenFileDialog1 As System.Windows.Forms.OpenFileDialog
    Friend WithEvents MenuStrip1 As System.Windows.Forms.MenuStrip
    Friend WithEvents ToolStripMenuItem1 As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents OpenToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents SaveToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents CloseToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents EditToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents DeviceToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents SaveFileDialog1 As System.Windows.Forms.SaveFileDialog
    Friend WithEvents lbl_file As System.Windows.Forms.Label
    Friend WithEvents Label2 As System.Windows.Forms.Label
    Friend WithEvents GroupBox1 As System.Windows.Forms.GroupBox
    Friend WithEvents btn_open As System.Windows.Forms.Button
    Friend WithEvents btn_hexview As System.Windows.Forms.Button
    ' Friend WithEvents At_isp_device1 As USB_Atmel_Device_ISP_Programmer.at_isp_device
    Friend WithEvents AboutToolStripMenuItem As System.Windows.Forms.ToolStripMenuItem
    Friend WithEvents chkbx_buff_init As CheckBox
    Friend WithEvents chkbx_8x1 As CheckBox
    Friend WithEvents btn_save_text As Button
    Friend WithEvents SaveFileDialog2 As SaveFileDialog
    Friend WithEvents chkbx_text_case As CheckBox
End Class
