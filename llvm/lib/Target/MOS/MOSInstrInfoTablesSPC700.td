def : ZPIRE< SPC700_ORA_ZeroPage, SPC700_ORA_Absolute >;
def : ZPIRE< SPC700_ORA_ZeroPageX, SPC700_ORA_AbsoluteX >;

def : ZPIRE< SPC700_AND_ZeroPage, SPC700_AND_Absolute >;
def : ZPIRE< SPC700_AND_ZeroPageX, SPC700_AND_AbsoluteX >;

def : ZPIRE< SPC700_EOR_ZeroPage, SPC700_EOR_Absolute >;
def : ZPIRE< SPC700_EOR_ZeroPageX, SPC700_EOR_AbsoluteX >;

def : ZPIRE< SPC700_CMP_ZeroPage, SPC700_CMP_Absolute >;
def : ZPIRE< SPC700_CMP_ZeroPageX, SPC700_CMP_AbsoluteX >;

def : ZPIRE< SPC700_ADC_ZeroPage, SPC700_ADC_Absolute >;
def : ZPIRE< SPC700_ADC_ZeroPageX, SPC700_ADC_AbsoluteX >;

def : ZPIRE< SPC700_SBC_ZeroPage, SPC700_SBC_Absolute >;
def : ZPIRE< SPC700_SBC_ZeroPageX, SPC700_SBC_AbsoluteX >;

def : ZPIRE< SPC700_STA_ZeroPage, SPC700_STA_Absolute >;
def : ZPIRE< SPC700_STA_ZeroPageX, SPC700_STA_AbsoluteX >;

def : ZPIRE< SPC700_LDA_ZeroPage, SPC700_LDA_Absolute >;
def : ZPIRE< SPC700_LDA_ZeroPageX, SPC700_LDA_AbsoluteX >;

def : ZPIRE< SPC700_STX_ZeroPage, SPC700_STX_Absolute >;

def : ZPIRE< SPC700_LDX_ZeroPage, SPC700_LDX_Absolute >;

def : ZPIRE< SPC700_STY_ZeroPage, SPC700_STY_Absolute >;

def : ZPIRE< SPC700_LDY_ZeroPage, SPC700_LDY_Absolute >;

def : ZPIRE< SPC700_ASL_ZeroPage, SPC700_ASL_Absolute >;

def : ZPIRE< SPC700_ROL_ZeroPage, SPC700_ROL_Absolute >;

def : ZPIRE< SPC700_LSR_ZeroPage, SPC700_LSR_Absolute >;

def : ZPIRE< SPC700_ROR_ZeroPage, SPC700_ROR_Absolute >;

def : ZPIRE< SPC700_DEC_ZeroPage, SPC700_DEC_Absolute >;

def : ZPIRE< SPC700_INC_ZeroPage, SPC700_INC_Absolute >;

def : ZPIRE< SPC700_CPX_ZeroPage, SPC700_CPX_Absolute >;

def : ZPIRE< SPC700_CPY_ZeroPage, SPC700_CPY_Absolute >;

/// ---------------------------------------------------------------------------
///
def MOSToSPC700MappingTable : GenericTable {
  let FilterClass = "MOSToSPC700MappingEntry";
  let Fields = ["From", "To"];

  let PrimaryKey = ["From"];
  let PrimaryKeyName = "getMOSToSPC700MappingEntry";
}
