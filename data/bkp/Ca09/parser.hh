# IDP  PATTERN OF INTERCONNECTIONS 
#
# This file describes a pattern of interconnections of ISBN dependency
# parser (idp). The format is similar to MALT parser format for feature models.
# It  describes the set of the previous state vectors to be connected to the
# latent variable state vector at each parser decision.
# Each line correspond to one type of connection and defines which state should be
# connected to the current state. Individual set of the weights is learned
# for each such a type.
# State is defined in respect to the current state of the parser
# (including the current partial dependency structure).
#
# IMPORTANT: 
# Read first *ih format description before starting to read this description
#
#
# Format:
# [FIND_LAST|FIND_FIRST] ORIG_STEP_TYPE ORIG_WORD_SPEC TARG_STEP_TYPE TARG_WORD_SPEC [FOLLOW_ACTION] [OFFSET]
# Where
# FIND_LAST/FIND_FIRST indicates whether the most recent state (FIND_LAST) or the 
# the most remote state in the history (FIND_FIRST) is to be preferred. If more 
# than one state correspond to the following specification then this preference 
# will be taken into account.
# ORIG_STEP_TYPE  defines types of steps to which this connection is applicable (ANY_STEP|SRL_STEP|SYNT_STEP)
# ORIG_WORD_SPEC defines a discrete function which returns a word in respect to 
# current position t
# TARG_STEP_TYPE  defines types of steps from which this connection can possibly go (ANY_STEP|SRL_STEP|SYNT_STEP)
# TARG_WORD_SPEC defines a discrete function which returns a word in respect to 
# some position t' in history
# If these 2 words are the same (refer to the same entity/index in the sentence)
# then latent state vectors t' and t  can be connected.
# The parser connects not more than one pair of states per type, taking into account 
# FIND_LAST/FIND_FIRST preference
#
#   ORIG_WORD_SPEC is a specification of the word at current position t :
#           SRC_STRUCTURE OFFSET_IN_STRUCTURE ARG_TRANSITION HEAD_TRANSITION_N R/L_CHILD_TRANSITIONS_N R/L_SIBLING_TRANSITIONS
#   i.e. it is the same as specification as used in our *ih fil
#   but without offset in the input sequence. 
#   Example of ORIG_WORD_SPEC: 
#   FIND_LAST ANY_STEP STACK 0 _ 0 1 0 - rightmost right child of the current TOP
#
#   TARG_WORD_SPEC is a specification of the word for word at some previous position t'. It has
#   simpler specification ORIG_WORD_SPEC
#           SRC_STRUCTURE OFFSET_IN_STRUCTURE
#   Example of TARG_WORD_SPEC:
#   SRL_STACK 0 - top of the SRL stack
#   
#   Additional restriction can be placed: only the parser state
#   which were followed by a particular decision can be connected to the current
#   state:
#   FOLLOW_ACTION SHIFT|LA|RA|RED|ANY, ANY - is default
#
#   Also we can place another restriction:
#   OFFSET defines that only the state with the time offset OFFSET can be considered.
#   If it matches - it will be connected, if not - no state will be connected
#   E.g. OFFSET of 1 is previous state. 2 - one more state before.

#SYNTAX model ==================

#Connect the most recent state with the same queue
FIND_LAST  SYNT_STEP INPUT 0 _ 0 0 0 SYNT_STEP INPUT 0 

#Connect the most recent state with the same element on top of the stack
FIND_LAST SYNT_STEP STACK 0 _ 0 0 0 SYNT_STEP STACK 0 

#Connect the most recent state where current rightmost right child of TOP was at TOP 
FIND_LAST SYNT_STEP STACK 0 _ 0 1 0 SYNT_STEP STACK 0 

#... leftmost left child of TOP was at TOP
FIND_LAST SYNT_STEP STACK 0 _ 0 -1 0 SYNT_STEP STACK 0

#... head of current TOP was at TOP
FIND_LAST SYNT_STEP STACK 0 _ 1 0 0 SYNT_STEP STACK 0

#... leftmost child of queue tail was at TOP
FIND_LAST SYNT_STEP INPUT 0 _ 0 -1 0 SYNT_STEP STACK 0

# This operation is added to guarantee that previous state is always
# connected to the current state, so that information can
# flow between any 2 states in the model
# Previous state is connected if its FRONT of queue is current TOP of
# the stack
FIND_LAST SYNT_STEP STACK 0 _ 0 0 0 SYNT_STEP INPUT 0 ANY 1

# SYNT from SRL model
FIND_LAST SYNT_STEP STACK 0 _ 0 0 0 SRL_STEP INPUT 0
FIND_LAST SYNT_STEP STACK 0 _ 0 0 0 SRL_STEP SRL_STACK 0


# SRL model
FIND_LAST SRL_STEP  INPUT 0 _ 0 0 0 SRL_STEP INPUT 0
FIND_LAST SRL_STEP SRL_STACK 0 _ 0 0 0 SRL_STEP SRL_STACK 0
FIND_LAST SRL_STEP SRL_STACK 0 _ 0 0 0 SRL_STEP INPUT 0
FIND_LAST SRL_STEP SRL_STACK 0 _ 1 0 0 SRL_STEP SRL_STACK 0

# SRL from SYNT model
FIND_LAST SRL_STEP SRL_STACK 0 _ 0 0 0 SYNT_STEP STACK 0
FIND_LAST SRL_STEP INPUT 0 _ 0 0 0 SYNT_STEP INPUT 0




